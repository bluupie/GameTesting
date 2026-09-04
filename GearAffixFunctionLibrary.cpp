#include "GearAffixFunctionLibrary.h"

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

// Walks a cumulative-weight line and returns the index whose bucket a random
// roll landed in. Returns -1 for an empty or all-zero-weight array.
int32 UGearAffixFunctionLibrary::PickWeightedIndex(const TArray<float>& Weights)
{
    float TotalWeight = 0.0f;
    for (float W : Weights)
    {
        TotalWeight += FMath::Max(W, 0.0f);
    }

    if (TotalWeight <= 0.0f)
    {
        return -1;
    }

    float Roll = FMath::FRandRange(0.0f, TotalWeight);
    float Cumulative = 0.0f;
    for (int32 i = 0; i < Weights.Num(); ++i)
    {
        Cumulative += FMath::Max(Weights[i], 0.0f);
        if (Roll <= Cumulative)
        {
            return i;
        }
    }
    return Weights.Num() - 1; // floating point edge case — fall back to last entry
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

TArray<FAffixDefinition> UGearAffixFunctionLibrary::GetEligibleAffixes(const TArray<FAffixDefinition>& AffixPool, EAffixType AffixType,
                                                                         EGearSlot Slot, int32 ItemLevel)
{
    TArray<FAffixDefinition> Result;
    for (const FAffixDefinition& Def : AffixPool)
    {
        if (Def.AffixType != AffixType || !Def.IsEligibleForSlot(Slot))
        {
            continue;
        }
        if (Def.GetUnlockedTierIndices(ItemLevel).Num() > 0)
        {
            Result.Add(Def);
        }
    }
    return Result;
}

int32 UGearAffixFunctionLibrary::RollTierIndexForAffix(const FAffixDefinition& Affix, int32 ItemLevel)
{
    TArray<int32> Unlocked = Affix.GetUnlockedTierIndices(ItemLevel);
    if (Unlocked.Num() == 0)
    {
        return -1;
    }

    TArray<float> Weights;
    Weights.Reserve(Unlocked.Num());
    for (int32 TierIdx : Unlocked)
    {
        Weights.Add(Affix.Tiers[TierIdx].Weight);
    }

    const int32 PickedLocal = PickWeightedIndex(Weights);
    return (PickedLocal >= 0) ? Unlocked[PickedLocal] : -1;
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
        TArray<FAffixDefinition> Candidates = GetEligibleAffixes(AffixPool, Type, Slot, ItemLevel);

        for (int32 i = 0; i < Count; ++i)
        {
            // Filter out groups already used on this item.
            TArray<FAffixDefinition> Remaining;
            for (const FAffixDefinition& Def : Candidates)
            {
                if (!UsedGroups.Contains(Def.AffixGroup))
                {
                    Remaining.Add(Def);
                }
            }
            if (Remaining.Num() == 0)
            {
                break; // ran out of distinct affix groups to roll — stop early rather than duplicate
            }

            TArray<float> Weights;
            Weights.Reserve(Remaining.Num());
            for (const FAffixDefinition& Def : Remaining)
            {
                Weights.Add(Def.SpawnWeight);
            }

            const int32 PickedIdx = PickWeightedIndex(Weights);
            if (PickedIdx < 0)
            {
                break;
            }

            const FAffixDefinition& Chosen = Remaining[PickedIdx];
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

FModifierPool UGearAffixFunctionLibrary::BuildModifierPoolFromGear(const FGearItem& Item, const TArray<FAffixDefinition>& AffixPool)
{
    FModifierPool Pool;

    auto ApplySide = [&](const TArray<FRolledAffix>& Affixes)
    {
        for (const FRolledAffix& Rolled : Affixes)
        {
            const FAffixDefinition* Def = FindDefinition(AffixPool, Rolled.AffixId);
            if (!Def)
            {
                continue; // AffixId not found in the pool passed in — data mismatch, skip rather than crash
            }

            switch (Def->ModApplication)
            {
                case EModifierApplication::Flat:
                    Pool.AddFlat(Def->StatType, Rolled.RolledValue);
                    break;
                case EModifierApplication::Increased:
                    Pool.AddIncreased(Def->StatType, Rolled.RolledValue);
                    break;
                case EModifierApplication::More:
                    Pool.AddMore(Def->StatType, Rolled.RolledValue);
                    break;
            }
        }
    };

    ApplySide(Item.PrefixAffixes);
    ApplySide(Item.SuffixAffixes);

    return Pool;
}

FString UGearAffixFunctionLibrary::DescribeAffix(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool)
{
    const FAffixDefinition* Def = FindDefinition(AffixPool, Rolled.AffixId);
    if (!Def)
    {
        return FString::Printf(TEXT("<unknown affix %s>"), *Rolled.AffixId.ToString());
    }
    return FString::Printf(*Def->DisplayFormat, Rolled.RolledValue);
}

// ---------------------------------------------------------------------------
// Example seed data
// ---------------------------------------------------------------------------
//
// Tier progression follows the requested pattern: Tier 9 is the weakest
// roll with no item level requirement, Tier 1 is the strongest roll and
// requires the highest item level. "Added Physical Damage" below matches
// the example exactly (T9: 1-4, T1: 100-200); the other affixes follow the
// same shape scaled to their own stat.

TArray<FAffixDefinition> UGearAffixFunctionLibrary::GetExampleAffixTable()
{
    TArray<FAffixDefinition> Table;

    // --- Prefix: Added Physical Damage (Weapon) ---------------------------
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Prefix_AddedPhysicalDamage");
        Def.DisplayFormat = TEXT("+%.0f to Physical Damage");
        Def.AffixType = EAffixType::Prefix;
        Def.StatType = EStatType::DamagePhysical;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("PhysDmgFlat");
        Def.AllowedSlots = { EGearSlot::Weapon };
        Def.SpawnWeight = 100.0f;
        Def.Tiers = {
            { 9, 1.0f,   4.0f,   1,  100.0f },
            { 8, 4.0f,   8.0f,   8,  90.0f  },
            { 7, 8.0f,   14.0f,  15, 80.0f  },
            { 6, 14.0f,  22.0f,  24, 65.0f  },
            { 5, 22.0f,  35.0f,  34, 50.0f  },
            { 4, 35.0f,  52.0f,  45, 38.0f  },
            { 3, 52.0f,  75.0f,  58, 26.0f  },
            { 2, 75.0f,  100.0f, 72, 14.0f  },
            { 1, 100.0f, 200.0f, 85, 6.0f   },
        };
        Table.Add(Def);
    }

    // --- Prefix: Increased Physical Damage % (Weapon) ----------------------
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Prefix_IncreasedPhysicalDamage");
        Def.DisplayFormat = TEXT("%.0f%% increased Physical Damage");
        Def.AffixType = EAffixType::Prefix;
        Def.StatType = EStatType::DamagePhysical;
        Def.ModApplication = EModifierApplication::Increased;
        Def.AffixGroup = FName("PhysDmgPercent");
        Def.AllowedSlots = { EGearSlot::Weapon };
        Def.SpawnWeight = 80.0f;
        // Stored as fractions (0.10 = 10%) to match FStatModifier::Percent's convention.
        Def.Tiers = {
            { 9, 0.10f, 0.15f, 1,  100.0f },
            { 8, 0.16f, 0.22f, 8,  90.0f  },
            { 7, 0.23f, 0.30f, 16, 80.0f  },
            { 6, 0.31f, 0.40f, 26, 65.0f  },
            { 5, 0.41f, 0.50f, 36, 50.0f  },
            { 4, 0.51f, 0.62f, 48, 38.0f  },
            { 3, 0.63f, 0.75f, 60, 26.0f  },
            { 2, 0.76f, 0.88f, 74, 14.0f  },
            { 1, 0.89f, 1.00f, 88, 6.0f   },
        };
        Table.Add(Def);
    }

    // --- Suffix: Critical Strike Chance % (Weapon) --------------------------
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Suffix_CriticalStrikeChance");
        Def.DisplayFormat = TEXT("Sharpened (+%.0f%% increased Critical Strike Chance)");
        Def.AffixType = EAffixType::Suffix;
        Def.StatType = EStatType::CriticalStrikeChance;
        Def.ModApplication = EModifierApplication::Increased;
        Def.AffixGroup = FName("CritChance");
        Def.AllowedSlots = { EGearSlot::Weapon };
        Def.SpawnWeight = 60.0f;
        Def.Tiers = {
            { 9, 0.10f, 0.15f, 1,  100.0f },
            { 8, 0.16f, 0.22f, 10, 90.0f  },
            { 7, 0.23f, 0.30f, 20, 80.0f  },
            { 6, 0.31f, 0.40f, 30, 65.0f  },
            { 5, 0.41f, 0.50f, 40, 50.0f  },
            { 4, 0.51f, 0.62f, 52, 38.0f  },
            { 3, 0.63f, 0.75f, 64, 26.0f  },
            { 2, 0.76f, 0.90f, 76, 14.0f  },
            { 1, 0.91f, 1.05f, 88, 6.0f   },
        };
        Table.Add(Def);
    }

    // --- Suffix: Attack Speed % (Weapon) -------------------------------------
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Suffix_AttackSpeed");
        Def.DisplayFormat = TEXT("of Haste (+%.0f%% increased Attack Speed)");
        Def.AffixType = EAffixType::Suffix;
        Def.StatType = EStatType::AttackSpeed;
        Def.ModApplication = EModifierApplication::Increased;
        Def.AffixGroup = FName("AttackSpeed");
        Def.AllowedSlots = { EGearSlot::Weapon };
        Def.SpawnWeight = 70.0f;
        Def.Tiers = {
            { 9, 0.03f, 0.05f, 1,  100.0f },
            { 8, 0.06f, 0.08f, 10, 90.0f  },
            { 7, 0.09f, 0.11f, 20, 80.0f  },
            { 6, 0.12f, 0.14f, 30, 65.0f  },
            { 5, 0.15f, 0.17f, 40, 50.0f  },
            { 4, 0.18f, 0.20f, 50, 38.0f  },
            { 3, 0.21f, 0.23f, 62, 26.0f  },
            { 2, 0.24f, 0.27f, 74, 14.0f  },
            { 1, 0.28f, 0.32f, 86, 6.0f   },
        };
        Table.Add(Def);
    }

    // --- Prefix: Maximum Life (Armor) ---------------------------------------
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Prefix_MaximumLife");
        Def.DisplayFormat = TEXT("of Vitality (+%.0f to Maximum Life)");
        Def.AffixType = EAffixType::Prefix;
        Def.StatType = EStatType::Life;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("Life");
        Def.AllowedSlots = { EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves, EGearSlot::Boots, EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring };
        Def.SpawnWeight = 100.0f;
        Def.Tiers = {
            { 9, 5.0f,   12.0f,  1,  100.0f },
            { 8, 13.0f,  22.0f,  8,  90.0f  },
            { 7, 23.0f,  35.0f,  16, 80.0f  },
            { 6, 36.0f,  52.0f,  26, 65.0f  },
            { 5, 53.0f,  72.0f,  36, 50.0f  },
            { 4, 73.0f,  95.0f,  48, 38.0f  },
            { 3, 96.0f,  125.0f, 60, 26.0f  },
            { 2, 126.0f, 160.0f, 74, 14.0f  },
            { 1, 161.0f, 200.0f, 88, 6.0f   },
        };
        Table.Add(Def);
    }
    {
        faffixDefinition Def;
        Def.AffixId = FName("Prefix_MaximumMana");
        Def.DisplayFormat = TEXT("of Clarity (+%.0f to Maximum Mana)");
        Def.AffixType = EAffixType::Prefix;
        Def.StatType = EStatType::Mana;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("Mana");
        Def.AllowedSlots = { EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves, EGearSlot::Boots, EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring };
        Def.SpawnWeight = 100.0f;
        Def.Tiers = {
            { 9, 5.0f,   10.0f,  1,  100.0f },
            { 8, 11.0f,  18.0f,  8,  90.0f  },
            { 7, 19.0f,  28.0f,  16, 80.0f  },
            { 6, 29.0f,  42.0f,  26, 65.0f  },
            { 5, 43.0f,  60.0f,  36, 50.0f  },
            { 4, 61.0f,  82.0f,  48, 38.0f  },
            { 3, 83.0f, 110.0f,   60,26.0f },
            {2 ,111.0f ,145.0f ,74 ,14.0f},
            {1 ,146.0f ,200.0f ,88 ,6.0f}
        };
        Table.Add(Def);
    }
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Prefix_MaximumEnergyShield");
        Def.DisplayFormat = TEXT("of the Aegis (+%.0f to Maximum Energy Shield)");
        Def.AffixType = EAffixType::Prefix;
        Def.StatType = EStatType::EnergyShield;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("EnergyShield");
        Def.AllowedSlots = { EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves, EGearSlot::Boots, EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring };
        Def.SpawnWeight = 100.0f;
        Def.Tiers = {
            { 9, 5.0f,   10.0f,  1,  100.0f },
            { 8, 11.0f,  18.0f,  8,  90.0f  },
            { 7, 19.0f,  28.0f,  16, 80.0f  },
            { 6, 29.0f,  42.0f,  26, 65.0f  },
            { 5, 43.0f,  60.0f,  36, 50.0f  },
            { 4, 61.0f,  82.0f, 48 ,38.0f},
            {3 ,83.0f ,110.0f ,60 ,26.0f},
            {2 ,111.0f ,145.0f ,74 ,14.0f},
            {1 ,146.0f ,200.0f ,88 ,6.0f}
        };
        Table.Add(Def);
    }


    // --- Suffix: Resistance (Armor / Amulet / Ring) ---------------------
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Suffix_FireResistance");
        Def.DisplayFormat = TEXT("of the Ember (+%.0f%% to Fire Resistance)");
        Def.AffixType = EAffixType::Suffix;
        Def.StatType = EStatType::FireResistance;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("FireRes");
        Def.AllowedSlots = { EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves, EGearSlot::Boots,
                              EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring };
        Def.SpawnWeight = 90.0f;
        Def.Tiers = {
            { 9, 5.0f,  8.0f,  1,  100.0f },
            { 8, 9.0f,  13.0f, 10, 90.0f  },
            { 7, 14.0f, 18.0f, 20, 80.0f  },
            { 6, 19.0f, 24.0f, 30, 65.0f  },
            { 5, 25.0f, 30.0f, 40, 50.0f  },
            { 4, 31.0f, 36.0f, 50, 38.0f  },
            { 3, 37.0f, 42.0f, 60, 26.0f  },
            { 2, 43.0f, 48.0f, 72, 14.0f  },
            { 1, 49.0f, 55.0f, 84, 6.0f   },
        };
        Table.Add(Def);
    }

    {
        FAffixDefinition Def;
        Def.AffixId = FName("Suffix_ColdResistance");
        Def.DisplayFormat = TEXT("of the Frost (+%.0f%% to Cold Resistance)");
        Def.AffixType = EAffixType::Suffix;
        Def.StatType = EStatType::ColdResistance;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("ColdRes");
        Def.AllowedSlots = { EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves, EGearSlot::Boots,
                              EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring };
        Def.SpawnWeight = 90.0f;
        Def.Tiers = {
            { 9, 5.0f,  8.0f,  1,  100.0f },
            { 8, 9.0f,  13.0f, 10, 90.0f  },
            { 7, 14.0f, 18.0f, 20, 80.0f  },
            { 6, 19.0f, 24.0f, 30, 65.0f  },
            { 5, 25.0f, 30.0f, 40, 50.0f  },
            { 4, 31.0f, 36.0f, 50, 38.0f  },
            { 3, 37.0f, 42.0f, 60, 26.0f  },
            { 2, 43.0f, 48.0f, 72, 14.0f  },
            { 1, 49.0f, 55.0f, 84, 6.0f   },
        };
        Table.Add(Def);
    }
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Suffix_LightningResistance");
        Def.DisplayFormat = TEXT("of the Storm (+%.0f%% to Lightning Resistance)");
        Def.AffixType = EAffixType::Suffix;
        Def.StatType = EStatType::LightningResistance;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("LightningRes");
        Def.AllowedSlots = { EGearSlot::Helmet, EGearSlot::Chest,EGearSlot::Gloves, EGearSlot::Boots, EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring };
        
        Def.SpawnWeight = 90.0f;
        Def.Tiers = {
            { 9, 5.0f,  8.0f,  1,  100.0f },
            { 8, 9.0f,  13.0f, 10, 90.0f  },
            { 7, 14.0f, 18.0f, 20, 80.0f  },
            { 6, 19.0f, 24.0f, 30, 65.0f  },
            { 5, 25.0f, 30.0f, 40, 50.0f  },
            { 4, 31.0f, 36.0f, 50, 38.0f  },
            { 3, 37.0f, 42.0f, 60, 26.0f  },
            { 2, 43.0f, 48.0f, 72, 14.0f  },
            { 1, 49.0f, 55.0f, 84, 6.0f   },
        };
        Table.Add(Def);
    }
    {
        FAffixDefinition Def;
        Def.AffixId = FName("Suffix_PoisonResistance");
        Def.DisplayFormat = TEXT("of the Venom (+%.0f%% to Poison Resistance)");
        Def.AffixType = EAffixType::Suffix;
        Def.StatType = EStatType::PoisonResistance;
        Def.ModApplication = EModifierApplication::Flat;
        Def.AffixGroup = FName("PoisonRes");
        Def.AllowedSlots = { EGearSlot::Helmet, EGearSlot::Chest,EGearSlot::Gloves, EGearSlot::Boots, EGearSlot::Belt, EGearSlot::Amulet, EGearSlot::Ring };
        
        Def.SpawnWeight = 90.0f;
        Def.Tiers = {
            { 9, 5.0f,  8.0f,  1,  100.0f },
            { 8, 9.0f,  13.0f, 10, 90.0f  },
            { 7, 14.0f, 18.0f, 20, 80.0f  },
            { 6, 19.0f, 24.0f, 30, 65.0f  },
            { 5, 25.0f, 30.0f, 40, 50.0f  },
            { 4, 31.0f, 36.0f, 50, 38.0f  },
            { 3, 37.0f, 42.0f, 60, 26.0f  },
            { 2, 43.0f, 48.0f, 72, 14.0f  },
            { 1, 49.0f, 55.0f, 84, 6.0f   },
        };
    }

    return Table;
}