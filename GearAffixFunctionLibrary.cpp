#include "GearAffixFunctionLibrary.h"
#include "Templates/Function.h" // TFunctionRef, used by the internal ApplyAffixesToContribution helper

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

const FAffixDefinition* UGearAffixFunctionLibrary::FindDefinition(const TArray<FAffixDefinition>& AffixPool, FName AffixId)
{
    for (const FAffixDefinition& Def : AffixPool)
    {
        if (Def.AffixId == AffixId)
        {
            return &Def;
        }
    }
    return nullptr;
}

const FAffixDefinition* UGearAffixFunctionLibrary::FindDefinitionIndexed(const TArray<FAffixDefinition>& AffixPool,
                                                                           const TMap<FName, int32>& AffixIndex, FName AffixId)
{
    if (const int32* Index = AffixIndex.Find(AffixId))
    {
        if (AffixPool.IsValidIndex(*Index) && AffixPool[*Index].AffixId == AffixId)
        {
            return &AffixPool[*Index];
        }
    }
    return nullptr;
}

TMap<FName, int32> UGearAffixFunctionLibrary::BuildAffixIndex(const TArray<FAffixDefinition>& AffixPool)
{
    TMap<FName, int32> Index;
    Index.Reserve(AffixPool.Num());
    for (int32 i = 0; i < AffixPool.Num(); ++i)
    {
        Index.Add(AffixPool[i].AffixId, i);
    }
    return Index;
}

// ---------------------------------------------------------------------------
// Rarity affix counts
// ---------------------------------------------------------------------------

void UGearAffixFunctionLibrary::GetAffixCountRangeForRarity(EItemRarity Rarity, int32& OutMinPrefixes, int32& OutMaxPrefixes,
                                                              int32& OutMinSuffixes, int32& OutMaxSuffixes)
{
    switch (Rarity)
    {
        case EItemRarity::Normal:
            OutMinPrefixes = 0; OutMaxPrefixes = 0;
            OutMinSuffixes = 0; OutMaxSuffixes = 0;
            break;
        case EItemRarity::Magic:
            OutMinPrefixes = 1; OutMaxPrefixes = 1;
            OutMinSuffixes = 1; OutMaxSuffixes = 1;
            break;
        case EItemRarity::Rare:
            OutMinPrefixes = 2; OutMaxPrefixes = 3;
            OutMinSuffixes = 2; OutMaxSuffixes = 3;
            break;
        case EItemRarity::Legendary:
            OutMinPrefixes = 3; OutMaxPrefixes = 3;
            OutMinSuffixes = 3; OutMaxSuffixes = 3;
            break;
        default:
            OutMinPrefixes = 0; OutMaxPrefixes = 0;
            OutMinSuffixes = 0; OutMaxSuffixes = 0;
            break;
    }
}

// ---------------------------------------------------------------------------
// Eligibility / rolling
// ---------------------------------------------------------------------------

TArray<int32> UGearAffixFunctionLibrary::GetEligibleAffixes(const TArray<FAffixDefinition>& AffixPool, EAffixType AffixType,
                                                              EGearSlot Slot, int32 ItemLevel)
{
    TArray<int32> Result;
    for (int32 i = 0; i < AffixPool.Num(); ++i)
    {
        const FAffixDefinition& Def = AffixPool[i];
        if (Def.AffixType != AffixType || !Def.IsEligibleForSlot(Slot))
        {
            continue;
        }
        if (Def.HasUnlockedTier(ItemLevel)) // non-allocating check — see FAffixDefinition::HasUnlockedTier
        {
            Result.Add(i);
        }
    }
    return Result;
}

int32 UGearAffixFunctionLibrary::RollTierIndexForAffix(const FAffixDefinition& Affix, int32 ItemLevel)
{
    // Single-pass weighted reservoir pick directly over Affix.Tiers — no
    // intermediate "unlocked indices" or "weights" array is built.
    return WeightedReservoirPick(Affix.Tiers.Num(), [&Affix, ItemLevel](int32 i)
    {
        const FAffixTier& Tier = Affix.Tiers[i];
        return (Tier.RequiredItemLevel <= ItemLevel) ? FMath::Max(Tier.Weight, 0.0f) : 0.0f;
    });
}

float UGearAffixFunctionLibrary::RollAffixValue(const FAffixTier& Tier)
{
    return FMath::FRandRange(Tier.MinRoll, Tier.MaxRoll);
}

// ---------------------------------------------------------------------------
// Item generation
// ---------------------------------------------------------------------------

FGearItem UGearAffixFunctionLibrary::GenerateGearItem(FName ItemId, const FString& ItemName, EGearSlot Slot, int32 ItemLevel,
                                                        EItemRarity Rarity, const TArray<FAffixDefinition>& AffixPool,
                                                        float RareFullAffixChance)
{
    FGearItem Item;
    Item.ItemId = ItemId;
    Item.ItemName = ItemName;
    Item.GearSlot = Slot;
    Item.ItemLevel = ItemLevel;
    Item.Rarity = Rarity;

    int32 MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes;
    GetAffixCountRangeForRarity(Rarity, MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes);

    if (MaxPrefixes == 0 && MaxSuffixes == 0)
    {
        return Item; // Normal-rarity items carry no affixes.
    }

    int32 NumPrefixes, NumSuffixes;
    if (MinPrefixes == MaxPrefixes && MinSuffixes == MaxSuffixes)
    {
        // Fixed count for this rarity (Magic 1+1, Legendary 3+3) — nothing to roll.
        NumPrefixes = MinPrefixes;
        NumSuffixes = MinSuffixes;
    }
    else
    {
        // Correlated roll: the item lands on the full affix count on BOTH
        // sides or the base count on BOTH sides, so a Rare never ends up
        // lopsided (e.g. 3 prefixes but only 2 suffixes).
        const bool bFullRoll = FMath::FRand() < RareFullAffixChance;
        NumPrefixes = bFullRoll ? MaxPrefixes : MinPrefixes;
        NumSuffixes = bFullRoll ? MaxSuffixes : MinSuffixes;
    }

    TSet<FName> UsedGroups; // shared across prefixes AND suffixes — no duplicate stat groups on one item

    auto RollSide = [&](EAffixType Type, int32 Count, TArray<FRolledAffix>& OutAffixes)
    {
        // Indices only — no struct copies. EligibleIndices is built once per
        // side (small, just int32s) and reused across every affix rolled for it.
        const TArray<int32> EligibleIndices = GetEligibleAffixes(AffixPool, Type, Slot, ItemLevel);
        if (EligibleIndices.Num() == 0)
        {
            return;
        }

        for (int32 i = 0; i < Count; ++i)
        {
            // Weighted reservoir pick over EligibleIndices, skipping any
            // affix whose group is already used (weight 0 = ineligible).
            // No "Remaining" array is built to do this filtering.
            const int32 LocalPick = WeightedReservoirPick(EligibleIndices.Num(), [&](int32 LocalIdx)
            {
                const FAffixDefinition& Def = AffixPool[EligibleIndices[LocalIdx]];
                return UsedGroups.Contains(Def.AffixGroup) ? 0.0f : FMath::Max(Def.SpawnWeight, 0.0f);
            });

            if (LocalPick < 0)
            {
                break; // ran out of distinct affix groups to roll — stop early rather than duplicate
            }

            const FAffixDefinition& Chosen = AffixPool[EligibleIndices[LocalPick]];
            const int32 TierIdx = RollTierIndexForAffix(Chosen, ItemLevel);
            if (TierIdx < 0)
            {
                continue; // shouldn't happen since Chosen came from GetEligibleAffixes, but stay defensive
            }

            const FAffixTier& Tier = Chosen.Tiers[TierIdx];

            FRolledAffix Rolled;
            Rolled.AffixId = Chosen.AffixId;
            Rolled.TierNumber = Tier.TierNumber;
            Rolled.RolledValue = RollAffixValue(Tier);
            OutAffixes.Add(Rolled);

            UsedGroups.Add(Chosen.AffixGroup);
        }
    };

    RollSide(EAffixType::Prefix, NumPrefixes, Item.PrefixAffixes);
    RollSide(EAffixType::Suffix, NumSuffixes, Item.SuffixAffixes);

    return Item;
}

// ---------------------------------------------------------------------------
// Pool building / display
// ---------------------------------------------------------------------------

static FGearStatContribution ApplyAffixesToContribution(const FGearItem& Item,
                                                          TFunctionRef<const FAffixDefinition*(FName)> Lookup)
{
    FGearStatContribution Contribution;

    auto ApplySide = [&](const TArray<FRolledAffix>& Affixes)
    {
        for (const FRolledAffix& Rolled : Affixes)
        {
            const FAffixDefinition* Def = Lookup(Rolled.AffixId);
            if (!Def)
            {
                continue; // AffixId not found in the pool passed in — data mismatch, skip rather than crash
            }

            if (Def->TargetType == EAffixTargetType::Attribute)
            {
                // Attribute affixes are always a flat addition — apply
                // BEFORE FCharacterStats::Recalculate, not through the pool.
                switch (Def->AttributeType)
                {
                    case EBaseAttributeType::Strength:     Contribution.BonusStrength += Rolled.RolledValue; break;
                    case EBaseAttributeType::Dexterity:    Contribution.BonusDexterity += Rolled.RolledValue; break;
                    case EBaseAttributeType::Intelligence: Contribution.BonusIntelligence += Rolled.RolledValue; break;
                }
                continue;
            }

            switch (Def->ModApplication)
            {
                case EModifierApplication::Flat:
                    Contribution.StatPool.AddFlat(Def->StatType, Rolled.RolledValue);
                    break;
                case EModifierApplication::Increased:
                    Contribution.StatPool.AddIncreased(Def->StatType, Rolled.RolledValue);
                    break;
                case EModifierApplication::More:
                    Contribution.StatPool.AddMore(Def->StatType, Rolled.RolledValue);
                    break;
            }
        }
    };

    ApplySide(Item.PrefixAffixes);
    ApplySide(Item.SuffixAffixes);

    return Contribution;
}

FGearStatContribution UGearAffixFunctionLibrary::BuildModifierPoolFromGear(const FGearItem& Item, const TArray<FAffixDefinition>& AffixPool)
{
    return ApplyAffixesToContribution(Item, [&AffixPool](FName Id) { return FindDefinition(AffixPool, Id); });
}

FGearStatContribution UGearAffixFunctionLibrary::BuildModifierPoolFromGearIndexed(const FGearItem& Item, const TArray<FAffixDefinition>& AffixPool,
                                                                                   const TMap<FName, int32>& AffixIndex)
{
    return ApplyAffixesToContribution(Item, [&AffixPool, &AffixIndex](FName Id) { return FindDefinitionIndexed(AffixPool, AffixIndex, Id); });
}

FString UGearAffixFunctionLibrary::DescribeAffix(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool)
{
    const FAffixDefinition* Def = FindDefinition(AffixPool, Rolled.AffixId);
    if (!Def)
    {
        return FString::Printf(TEXT("<unknown affix %s>"), *Rolled.AffixId.ToString());
    }
    const float DisplayValue = Def->bIsPercentageValue ? Rolled.RolledValue * 100.0f : Rolled.RolledValue;
    return FString::Printf(*Def->DisplayFormat, DisplayValue);
}

FString UGearAffixFunctionLibrary::DescribeAffixIndexed(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool,
                                                          const TMap<FName, int32>& AffixIndex)
{
    const FAffixDefinition* Def = FindDefinitionIndexed(AffixPool, AffixIndex, Rolled.AffixId);
    if (!Def)
    {
        return FString::Printf(TEXT("<unknown affix %s>"), *Rolled.AffixId.ToString());
    }
    const float DisplayValue = Def->bIsPercentageValue ? Rolled.RolledValue * 100.0f : Rolled.RolledValue;
    return FString::Printf(*Def->DisplayFormat, DisplayValue);
}

TArray<FAffixTier> UGearAffixFunctionLibrary::BuildTierProgression(float Tier9Min, float Tier9Max, float Tier1Min, float Tier1Max,
                                                                     int32 Tier9ItemLevel, int32 Tier1ItemLevel,
                                                                     float Tier9Weight, float Tier1Weight)
{
    TArray<FAffixTier> Tiers;
    Tiers.Reserve(9);
    for (int32 TierNumber = 9; TierNumber >= 1; --TierNumber)
    {
        const float Alpha = static_cast<float>(9 - TierNumber) / 8.0f; // 0 at Tier 9, 1 at Tier 1

        FAffixTier Tier;
        Tier.TierNumber = TierNumber;
        Tier.MinRoll = FMath::Lerp(Tier9Min, Tier1Min, Alpha);
        Tier.MaxRoll = FMath::Lerp(Tier9Max, Tier1Max, Alpha);
        Tier.RequiredItemLevel = FMath::RoundToInt(FMath::Lerp(static_cast<float>(Tier9ItemLevel), static_cast<float>(Tier1ItemLevel), Alpha));
        Tier.Weight = FMath::Lerp(Tier9Weight, Tier1Weight, Alpha);
        Tiers.Add(Tier);
    }
    return Tiers;
}

int32 UGearAffixFunctionLibrary::MakeSlotMask(const TArray<EGearSlot>& Slots)
{
    int32 Mask = 0;
    for (EGearSlot Slot : Slots)
    {
        Mask |= GetGearSlotBit(Slot);
    }
    return Mask;
}

int32 UGearAffixFunctionLibrary::GetAllGearSlotsMask()
{
    return MakeSlotMask({ EGearSlot::Weapon, EGearSlot::OffHand, EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves,
                           EGearSlot::Boots, EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring });
}

int32 UGearAffixFunctionLibrary::GetNonWeaponGearSlotsMask()
{
    return MakeSlotMask({ EGearSlot::OffHand, EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves,
                           EGearSlot::Boots, EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring });
}

// ---------------------------------------------------------------------------
// Example seed data
// ---------------------------------------------------------------------------
//
// One affix per EStatType that carries a Prefix/Suffix designation, plus
// Strength/Dexterity/Intelligence as attribute-targeting suffixes. Skipped
// intentionally, per their annotations: DodgeChance (not an affix — solely
// derived from Evasion), CooldownReduction and AoeRadius (undetermined
// design, no Prefix/Suffix assigned yet). EnergyShield/BlockAmount/Accuracy
// are not present here as they've been removed from EStatType.
//
// Every affix uses the same Tier 9 -> Tier 1 item level span (1 -> 85) and
// weight falloff (100 -> 6) via BuildTierProgression; only the roll range
// differs per stat. Adjust the shared constants below, or pass different
// values per affix, to change that.

TArray<FAffixDefinition> UGearAffixFunctionLibrary::GetExampleAffixTable()
{
    TArray<FAffixDefinition> Table;

    const int32 AllSlots = GetAllGearSlotsMask();
    const int32 NonWeaponSlots = GetNonWeaponGearSlotsMask();
    const int32 WeaponGlovesRing = MakeSlotMask({ EGearSlot::Weapon, EGearSlot::Gloves, EGearSlot::Ring });
    const int32 WeaponRing = MakeSlotMask({ EGearSlot::Weapon, EGearSlot::Ring });
    const int32 WeaponRingGloves = MakeSlotMask({ EGearSlot::Weapon, EGearSlot::Ring, EGearSlot::Gloves });
    const int32 BootsOnly = MakeSlotMask({ EGearSlot::Boots });
    const int32 OffHandOnly = MakeSlotMask({ EGearSlot::OffHand });
    const int32 WeaponOnly = MakeSlotMask({ EGearSlot::Weapon });

    const int32 Tier9Ilvl = 1;
    const int32 Tier1Ilvl = 85;
    const float Tier9Weight = 100.0f;
    const float Tier1Weight = 6.0f;

    auto AddStatAffix = [&](FName Id, const FString& Format, EAffixType Type, EStatType Stat, EModifierApplication App,
                             FName Group, int32 SlotsMask, float SpawnWeight,
                             float T9Min, float T9Max, float T1Min, float T1Max, bool bIsPercent = false)
    {
        FAffixDefinition Def;
        Def.AffixId = Id;
        Def.DisplayFormat = Format;
        Def.AffixType = Type;
        Def.TargetType = EAffixTargetType::Stat;
        Def.StatType = Stat;
        Def.ModApplication = App;
        Def.AffixGroup = Group;
        Def.AllowedSlotsMask = SlotsMask;
        Def.SpawnWeight = SpawnWeight;
        Def.bIsPercentageValue = bIsPercent;
        Def.Tiers = BuildTierProgression(T9Min, T9Max, T1Min, T1Max, Tier9Ilvl, Tier1Ilvl, Tier9Weight, Tier1Weight);
        Table.Add(Def);
    };

    auto AddAttributeAffix = [&](FName Id, const FString& Format, EBaseAttributeType Attr, FName Group,
                                  float SpawnWeight, float T9Min, float T9Max, float T1Min, float T1Max)
    {
        FAffixDefinition Def;
        Def.AffixId = Id;
        Def.DisplayFormat = Format;
        Def.AffixType = EAffixType::Suffix;
        Def.TargetType = EAffixTargetType::Attribute;
        Def.AttributeType = Attr;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = Group;
        Def.AllowedSlotsMask = AllSlots;
        Def.SpawnWeight = SpawnWeight;
        Def.bIsPercentageValue = false;
        Def.Tiers = BuildTierProgression(T9Min, T9Max, T1Min, T1Max, Tier9Ilvl, Tier1Ilvl, Tier9Weight, Tier1Weight);
        Table.Add(Def);
    };

    // --- Attributes (Suffix -- All) -----------------------------------------
    AddAttributeAffix(FName("Suffix_Strength"), TEXT("of the Bear (+%.0f to Strength)"),
                       EBaseAttributeType::Strength, FName("AttrStrength"), 100.0f, 3.0f, 6.0f, 40.0f, 65.0f);
    AddAttributeAffix(FName("Suffix_Dexterity"), TEXT("of the Fox (+%.0f to Dexterity)"),
                       EBaseAttributeType::Dexterity, FName("AttrDexterity"), 100.0f, 3.0f, 6.0f, 40.0f, 65.0f);
    AddAttributeAffix(FName("Suffix_Intelligence"), TEXT("of the Owl (+%.0f to Intelligence)"),
                       EBaseAttributeType::Intelligence, FName("AttrIntelligence"), 100.0f, 3.0f, 6.0f, 40.0f, 65.0f);

    // --- Prefixes ------------------------------------------------------------
    AddStatAffix(FName("Prefix_Life"), TEXT("+%.0f to Maximum Life"), EAffixType::Prefix,
                 EStatType::Life, EModifierApplication::Flat, FName("Life"), NonWeaponSlots, 100.0f,
                 5.0f, 12.0f, 161.0f, 200.0f);
    AddStatAffix(FName("Prefix_Mana"), TEXT("+%.0f to Maximum Mana"), EAffixType::Prefix,
                 EStatType::Mana, EModifierApplication::Flat, FName("Mana"), AllSlots, 90.0f,
                 4.0f, 8.0f, 90.0f, 120.0f);
    AddStatAffix(FName("Prefix_Barrier"), TEXT("+%.0f to Barrier"), EAffixType::Prefix,
                 EStatType::Barrier, EModifierApplication::Flat, FName("Barrier"), NonWeaponSlots, 70.0f,
                 4.0f, 10.0f, 120.0f, 160.0f);
    AddStatAffix(FName("Prefix_AddedPhysicalDamage"), TEXT("+%.0f to Physical Damage"), EAffixType::Prefix,
                 EStatType::DamagePhysical, EModifierApplication::Flat, FName("PhysDmg"), WeaponGlovesRing, 100.0f,
                 1.0f, 4.0f, 100.0f, 200.0f);
    AddStatAffix(FName("Prefix_AddedFireDamage"), TEXT("+%.0f to Fire Damage"), EAffixType::Prefix,
                 EStatType::DamageFire, EModifierApplication::Flat, FName("FireDmg"), WeaponGlovesRing, 80.0f,
                 2.0f, 5.0f, 80.0f, 140.0f);
    AddStatAffix(FName("Prefix_AddedColdDamage"), TEXT("+%.0f to Cold Damage"), EAffixType::Prefix,
                 EStatType::DamageCold, EModifierApplication::Flat, FName("ColdDmg"), WeaponGlovesRing, 80.0f,
                 2.0f, 5.0f, 80.0f, 140.0f);
    AddStatAffix(FName("Prefix_AddedLightningDamage"), TEXT("+%.0f to Lightning Damage"), EAffixType::Prefix,
                 EStatType::DamageLightning, EModifierApplication::Flat, FName("LightningDmg"), WeaponGlovesRing, 80.0f,
                 3.0f, 8.0f, 100.0f, 180.0f);
    AddStatAffix(FName("Prefix_AddedPoisonDamage"), TEXT("+%.0f to Poison Damage"), EAffixType::Prefix,
                 EStatType::DamagePoison, EModifierApplication::Flat, FName("PoisonDmg"), WeaponGlovesRing, 70.0f,
                 1.0f, 3.0f, 40.0f, 70.0f);
    AddStatAffix(FName("Prefix_MovementSpeed"), TEXT("+%.0f%% increased Movement Speed"), EAffixType::Prefix,
                 EStatType::MovementSpeed, EModifierApplication::Increased, FName("MoveSpeed"), BootsOnly, 100.0f,
                 0.02f, 0.04f, 0.20f, 0.30f, /*bIsPercent=*/true);
    AddStatAffix(FName("Prefix_Armour"), TEXT("+%.0f to Armour"), EAffixType::Prefix,
                 EStatType::Armour, EModifierApplication::Flat, FName("Armour"), NonWeaponSlots, 100.0f,
                 3.0f, 8.0f, 150.0f, 220.0f);
    AddStatAffix(FName("Prefix_FireResistance"), TEXT("+%.0f%% to Fire Resistance"), EAffixType::Prefix,
                 EStatType::FireResistance, EModifierApplication::Flat, FName("FireRes"), NonWeaponSlots, 90.0f,
                 5.0f, 8.0f, 49.0f, 55.0f, /*bIsPercent=*/true);
    AddStatAffix(FName("Prefix_ColdResistance"), TEXT("+%.0f%% to Cold Resistance"), EAffixType::Prefix,
                 EStatType::ColdResistance, EModifierApplication::Flat, FName("ColdRes"), NonWeaponSlots, 90.0f,
                 5.0f, 8.0f, 49.0f, 55.0f, /*bIsPercent=*/true);
    AddStatAffix(FName("Prefix_LightningResistance"), TEXT("+%.0f%% to Lightning Resistance"), EAffixType::Prefix,
                 EStatType::LightningResistance, EModifierApplication::Flat, FName("LightningRes"), NonWeaponSlots, 90.0f,
                 5.0f, 8.0f, 49.0f, 55.0f, /*bIsPercent=*/true);
    AddStatAffix(FName("Prefix_PoisonResistance"), TEXT("+%.0f%% to Poison Resistance"), EAffixType::Prefix,
                 EStatType::PoisonResistance, EModifierApplication::Flat, FName("PoisonRes"), NonWeaponSlots, 90.0f,
                 5.0f, 8.0f, 49.0f, 55.0f, /*bIsPercent=*/true);
    AddStatAffix(FName("Prefix_Evasion"), TEXT("+%.0f to Evasion"), EAffixType::Prefix,
                 EStatType::Evasion, EModifierApplication::Flat, FName("Evasion"), NonWeaponSlots, 100.0f,
                 3.0f, 8.0f, 150.0f, 220.0f);
    AddStatAffix(FName("Prefix_BlockChance"), TEXT("+%.0f%% increased Block Chance"), EAffixType::Prefix,
                 EStatType::BlockChance, EModifierApplication::Flat, FName("BlockChance"), OffHandOnly, 60.0f,
                 0.02f, 0.04f, 0.15f, 0.20f, /*bIsPercent=*/true);
    AddStatAffix(FName("Prefix_SpellBlockChance"), TEXT("+%.0f%% increased Spell Block Chance"), EAffixType::Prefix,
                 EStatType::SpellBlock, EModifierApplication::Flat, FName("SpellBlock"), OffHandOnly, 60.0f,
                 0.02f, 0.04f, 0.15f, 0.20f, /*bIsPercent=*/true);
    AddStatAffix(FName("Prefix_SpellDamage"), TEXT("+%.0f to Spell Damage"), EAffixType::Prefix,
                 EStatType::SpellDamage, EModifierApplication::Flat, FName("SpellDamage"), WeaponOnly, 80.0f,
                 5.0f, 10.0f, 100.0f, 180.0f);

    // --- Suffixes --------------------------------------------------------------
    AddStatAffix(FName("Suffix_CriticalStrikeChance"), TEXT("Sharpened (+%.0f%% increased Critical Strike Chance)"), EAffixType::Suffix,
                 EStatType::CriticalStrikeChance, EModifierApplication::Increased, FName("CritChance"), WeaponRing, 60.0f,
                 0.10f, 0.15f, 0.89f, 1.00f, /*bIsPercent=*/true);
    AddStatAffix(FName("Suffix_CriticalStrikeMultiplier"), TEXT("Deadly (+%.0f%% to Critical Strike Multiplier)"), EAffixType::Suffix,
                 EStatType::CriticalStrikeMultiplier, EModifierApplication::Flat, FName("CritMulti"), WeaponRing, 60.0f,
                 0.05f, 0.10f, 0.50f, 0.80f, /*bIsPercent=*/true);
    AddStatAffix(FName("Suffix_AttackSpeed"), TEXT("of Haste (+%.0f%% increased Attack Speed)"), EAffixType::Suffix,
                 EStatType::AttackSpeed, EModifierApplication::Increased, FName("AttackSpeed"), WeaponRingGloves, 70.0f,
                 0.03f, 0.05f, 0.28f, 0.32f, /*bIsPercent=*/true);
    AddStatAffix(FName("Suffix_CastSpeed"), TEXT("of Alacrity (+%.0f%% increased Cast Speed)"), EAffixType::Suffix,
                 EStatType::CastSpeed, EModifierApplication::Increased, FName("CastSpeed"), WeaponRingGloves, 70.0f,
                 0.03f, 0.05f, 0.28f, 0.32f, /*bIsPercent=*/true);
    AddStatAffix(FName("Suffix_LifeLeech"), TEXT("of the Leech (+%.1f%% of Physical Damage Leeched as Life)"), EAffixType::Suffix,
                 EStatType::LifeLeech, EModifierApplication::Flat, FName("LifeLeech"), WeaponGlovesRing, 50.0f,
                 0.002f, 0.004f, 0.02f, 0.03f, /*bIsPercent=*/true);
    AddStatAffix(FName("Suffix_ManaLeech"), TEXT("of the Siphon (+%.1f%% of Physical Damage Leeched as Mana)"), EAffixType::Suffix,
                 EStatType::ManaLeech, EModifierApplication::Flat, FName("ManaLeech"), WeaponGlovesRing, 50.0f,
                 0.002f, 0.004f, 0.02f, 0.03f, /*bIsPercent=*/true);
    AddStatAffix(FName("Suffix_HealthRegen"), TEXT("of Recovery (+%.0f Life Regenerated per second)"), EAffixType::Suffix,
                 EStatType::HealthRegen, EModifierApplication::Flat, FName("HealthRegen"), NonWeaponSlots, 80.0f,
                 1.0f, 3.0f, 20.0f, 35.0f);
    AddStatAffix(FName("Suffix_ManaRegen"), TEXT("of Clarity (+%.0f Mana Regenerated per second)"), EAffixType::Suffix,
                 EStatType::ManaRegen, EModifierApplication::Flat, FName("ManaRegen"), NonWeaponSlots, 80.0f,
                 1.0f, 2.0f, 12.0f, 20.0f);

    return Table;
}