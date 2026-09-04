#include "GearBaseFunctionLibrary.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float UGearBaseFunctionLibrary::LerpRankValue(int32 Rank, int32 TotalRanks, float MinValue, float MaxValue)
{
    const float Alpha = (TotalRanks <= 1) ? 0.0f : static_cast<float>(Rank - 1) / static_cast<float>(TotalRanks - 1);
    return FMath::Lerp(MinValue, MaxValue, Alpha);
}

FGearBaseLevelProgression UGearBaseFunctionLibrary::GetBaseLevelProgression(int32 Rank, int32 TotalRanks,
                                                                             int32 MinCharacterLevel, int32 MaxCharacterLevel,
                                                                             int32 MinItemLevel, int32 MaxItemLevel)
{
    FGearBaseLevelProgression Progression;
    Progression.RequiredCharacterLevel = FMath::RoundToInt(LerpRankValue(Rank, TotalRanks,
                                                                          static_cast<float>(MinCharacterLevel), static_cast<float>(MaxCharacterLevel)));
    Progression.BaseItemLevel = FMath::RoundToInt(LerpRankValue(Rank, TotalRanks,
                                                                 static_cast<float>(MinItemLevel), static_cast<float>(MaxItemLevel)));
    return Progression;
}

// ---------------------------------------------------------------------------
// Procedural naming
// ---------------------------------------------------------------------------
//
// Rather than hand-authoring 108+15+5 unique strings, each defense type gets
// a 9-entry (or 5-entry, for shields) adjective progression from weakest to
// strongest, combined with a per-slot noun. Swap these arrays for real
// authored names whenever — BaseId stays stable either way since it's built
// from the defense/slot/rank enums, not the display name.

namespace GearBaseNaming
{
    const TArray<FString>& GetArmourAdjectives(EDefenseType DefenseType)
    {
        static const TArray<FString> Armour  = { TEXT("Rusted"), TEXT("Worn"), TEXT("Bronze"), TEXT("Iron"), TEXT("Steel"),
                                                   TEXT("Reinforced"), TEXT("Knight's"), TEXT("Dragon"), TEXT("Exalted") };
        static const TArray<FString> Evasion = { TEXT("Ragged"), TEXT("Tattered"), TEXT("Leather"), TEXT("Ranger's"), TEXT("Shadow"),
                                                   TEXT("Nightstalker's"), TEXT("Phantom"), TEXT("Wraith"), TEXT("Exalted") };
        static const TArray<FString> Barrier = { TEXT("Frayed"), TEXT("Cracked"), TEXT("Silk"), TEXT("Woven"), TEXT("Arcane"),
                                                   TEXT("Sorcerer's"), TEXT("Astral"), TEXT("Celestial"), TEXT("Exalted") };
        switch (DefenseType)
        {
            case EDefenseType::Evasion: return Evasion;
            case EDefenseType::Barrier: return Barrier;
            default:                    return Armour;
        }
    }

    const TArray<FString>& GetShieldAdjectives(EDefenseType DefenseType)
    {
        static const TArray<FString> Armour  = { TEXT("Rusted"), TEXT("Iron"), TEXT("Steel"), TEXT("Knight's"), TEXT("Exalted") };
        static const TArray<FString> Evasion = { TEXT("Ragged"), TEXT("Leather"), TEXT("Ranger's"), TEXT("Phantom"), TEXT("Exalted") };
        static const TArray<FString> Barrier = { TEXT("Cracked"), TEXT("Woven"), TEXT("Arcane"), TEXT("Astral"), TEXT("Exalted") };
        switch (DefenseType)
        {
            case EDefenseType::Evasion: return Evasion;
            case EDefenseType::Barrier: return Barrier;
            default:                    return Armour;
        }
    }

    FString GetArmourSlotNoun(EDefenseType DefenseType, EGearSlot Slot)
    {
        switch (DefenseType)
        {
            case EDefenseType::Evasion:
                switch (Slot)
                {
                    case EGearSlot::Helmet: return TEXT("Cap");
                    case EGearSlot::Chest:  return TEXT("Vest");
                    case EGearSlot::Gloves: return TEXT("Gloves");
                    case EGearSlot::Boots:  return TEXT("Boots");
                    default:                return TEXT("Gear");
                }
            case EDefenseType::Barrier:
                switch (Slot)
                {
                    case EGearSlot::Helmet: return TEXT("Circlet");
                    case EGearSlot::Chest:  return TEXT("Robe");
                    case EGearSlot::Gloves: return TEXT("Wraps");
                    case EGearSlot::Boots:  return TEXT("Slippers");
                    default:                return TEXT("Gear");
                }
            default: // Armour
                switch (Slot)
                {
                    case EGearSlot::Helmet: return TEXT("Helm");
                    case EGearSlot::Chest:  return TEXT("Platebody");
                    case EGearSlot::Gloves: return TEXT("Gauntlets");
                    case EGearSlot::Boots:  return TEXT("Greaves");
                    default:                return TEXT("Gear");
                }
        }
    }

    FString GetShieldNoun(EDefenseType DefenseType)
    {
        switch (DefenseType)
        {
            case EDefenseType::Evasion: return TEXT("Buckler");
            case EDefenseType::Barrier: return TEXT("Ward");
            default:                    return TEXT("Bulwark");
        }
    }

    FString GetSlotIdLabel(EGearSlot Slot)
    {
        switch (Slot)
        {
            case EGearSlot::Helmet: return TEXT("Helmet");
            case EGearSlot::Chest:  return TEXT("Chest");
            case EGearSlot::Gloves: return TEXT("Gloves");
            case EGearSlot::Boots:  return TEXT("Boots");
            default:                return TEXT("Slot");
        }
    }

    FString GetDefenseIdLabel(EDefenseType DefenseType)
    {
        switch (DefenseType)
        {
            case EDefenseType::Evasion: return TEXT("Evasion");
            case EDefenseType::Barrier: return TEXT("Barrier");
            default:                    return TEXT("Armour");
        }
    }
}

FString UGearBaseFunctionLibrary::BuildArmourBaseName(EDefenseType DefenseType, EGearSlot Slot, int32 Rank)
{
    const TArray<FString>& Adjectives = GearBaseNaming::GetArmourAdjectives(DefenseType);
    const FString& Adjective = Adjectives.IsValidIndex(Rank - 1) ? Adjectives[Rank - 1] : TEXT("Unknown");
    return FString::Printf(TEXT("%s %s"), *Adjective, *GearBaseNaming::GetArmourSlotNoun(DefenseType, Slot));
}

FString UGearBaseFunctionLibrary::BuildShieldBaseName(EDefenseType DefenseType, int32 Rank)
{
    const TArray<FString>& Adjectives = GearBaseNaming::GetShieldAdjectives(DefenseType);
    const FString& Adjective = Adjectives.IsValidIndex(Rank - 1) ? Adjectives[Rank - 1] : TEXT("Unknown");
    return FString::Printf(TEXT("%s %s"), *Adjective, *GearBaseNaming::GetShieldNoun(DefenseType));
}

// ---------------------------------------------------------------------------
// Example base tables
// ---------------------------------------------------------------------------

TArray<FGearBaseItem> UGearBaseFunctionLibrary::GetExampleArmourBaseTable()
{
    TArray<FGearBaseItem> Table;
    Table.Reserve(9 * 4 * 3);

    const TArray<EDefenseType> DefenseTypes = { EDefenseType::Armour, EDefenseType::Evasion, EDefenseType::Barrier };
    const TArray<EGearSlot> ArmourSlots = { EGearSlot::Helmet, EGearSlot::Chest, EGearSlot::Gloves, EGearSlot::Boots };

    // Reference defense-value curve (applies to Chest, weight 1.0); other
    // slots scale down from it — Chest grants the most, Gloves the least,
    // matching typical ARPG slot weighting.
    const float MinDefenseAtRank1 = 12.0f;
    const float MaxDefenseAtRank9 = 420.0f;

    auto GetSlotWeight = [](EGearSlot Slot) -> float
    {
        switch (Slot)
        {
            case EGearSlot::Chest:  return 1.0f;
            case EGearSlot::Helmet: return 0.65f;
            case EGearSlot::Boots:  return 0.55f;
            case EGearSlot::Gloves: return 0.50f;
            default:                return 1.0f;
        }
    };

    for (EDefenseType DefenseType : DefenseTypes)
    {
        for (EGearSlot Slot : ArmourSlots)
        {
            for (int32 Rank = 1; Rank <= 9; ++Rank)
            {
                const FGearBaseLevelProgression Levels = GetBaseLevelProgression(Rank, 9);

                FGearBaseItem Base;
                Base.BaseId = FName(*FString::Printf(TEXT("%s_%s_Rank%d"),
                                                       *GearBaseNaming::GetDefenseIdLabel(DefenseType),
                                                       *GearBaseNaming::GetSlotIdLabel(Slot), Rank));
                Base.BaseName = BuildArmourBaseName(DefenseType, Slot, Rank);
                Base.Slot = Slot;
                Base.DefenseType = DefenseType;
                Base.bHasDefenseType = true;
                Base.RequiredCharacterLevel = Levels.RequiredCharacterLevel;
                Base.BaseItemLevel = Levels.BaseItemLevel;
                Base.BaseDefenseValue = FMath::RoundToFloat(LerpRankValue(Rank, 9, MinDefenseAtRank1, MaxDefenseAtRank9) * GetSlotWeight(Slot));
                Base.BaseBlockChance = 0.0f;

                Table.Add(Base);
            }
        }
    }

    return Table;
}

TArray<FGearBaseItem> UGearBaseFunctionLibrary::GetExampleShieldBaseTable()
{
    TArray<FGearBaseItem> Table;
    Table.Reserve(5 * 3);

    const TArray<EDefenseType> DefenseTypes = { EDefenseType::Armour, EDefenseType::Evasion, EDefenseType::Barrier };

    const float MinDefenseAtRank1 = 20.0f;
    const float MaxDefenseAtRank5 = 380.0f;
    const float MinBlockAtRank1 = 0.04f;  // fraction — matches the affix system's BlockChance convention
    const float MaxBlockAtRank5 = 0.28f;

    for (EDefenseType DefenseType : DefenseTypes)
    {
        for (int32 Rank = 1; Rank <= 5; ++Rank)
        {
            const FGearBaseLevelProgression Levels = GetBaseLevelProgression(Rank, 5);

            FGearBaseItem Base;
            Base.BaseId = FName(*FString::Printf(TEXT("Shield_%s_Rank%d"), *GearBaseNaming::GetDefenseIdLabel(DefenseType), Rank));
            Base.BaseName = BuildShieldBaseName(DefenseType, Rank);
            Base.Slot = EGearSlot::Shield;
            Base.DefenseType = DefenseType;
            Base.bHasDefenseType = true;
            Base.OffHandType = EOffHandType::Shield;
            Base.RequiredCharacterLevel = Levels.RequiredCharacterLevel;
            Base.BaseItemLevel = Levels.BaseItemLevel;
            Base.BaseDefenseValue = FMath::RoundToFloat(LerpRankValue(Rank, 5, MinDefenseAtRank1, MaxDefenseAtRank5));
            Base.BaseBlockChance = LerpRankValue(Rank, 5, MinBlockAtRank1, MaxBlockAtRank5);

            Table.Add(Base);
        }
    }

    return Table;
}

TArray<FGearBaseItem> UGearBaseFunctionLibrary::GetExampleQuiverBaseTable()
{
    TArray<FGearBaseItem> Table;
    Table.Reserve(5);

    static const TArray<FString> QuiverNames = { TEXT("Worn Quiver"), TEXT("Serrated Quiver"), TEXT("Reinforced Quiver"),
                                                   TEXT("Hunter's Quiver"), TEXT("Exalted Quiver") };

    for (int32 Rank = 1; Rank <= 5; ++Rank)
    {
        const FGearBaseLevelProgression Levels = GetBaseLevelProgression(Rank, 5);

        FGearBaseItem Base;
        Base.BaseId = FName(*FString::Printf(TEXT("Quiver_Rank%d"), Rank));
        Base.BaseName = QuiverNames.IsValidIndex(Rank - 1) ? QuiverNames[Rank - 1] : TEXT("Quiver");
        Base.Slot = EGearSlot::Quiver;
        Base.bHasDefenseType = false; // quivers carry no defense family
        Base.OffHandType = EOffHandType::Quiver;
        Base.RequiredCharacterLevel = Levels.RequiredCharacterLevel;
        Base.BaseItemLevel = Levels.BaseItemLevel;
        Base.BaseDefenseValue = 0.0f;
        Base.BaseBlockChance = 0.0f;

        Table.Add(Base);
    }

    return Table;
}

TArray<FGearBaseItem> UGearBaseFunctionLibrary::GetAllExampleGearBases()
{
    TArray<FGearBaseItem> All = GetExampleArmourBaseTable();
    All.Append(GetExampleShieldBaseTable());
    All.Append(GetExampleQuiverBaseTable());
    return All;
}

// ---------------------------------------------------------------------------
// Defense type affix restrictions
// ---------------------------------------------------------------------------
//
// Single source of truth for which affix groups can't roll on which defense
// type. Add a row to add a restriction; delete a row to remove one — no
// other function in this file needs to change either way.

TArray<FDefenseTypeAffixRestriction> UGearBaseFunctionLibrary::GetExampleDefenseTypeRestrictions()
{
    TArray<FDefenseTypeAffixRestriction> Restrictions;

    auto AddRestriction = [&Restrictions](EDefenseType DefenseType, FName Group)
    {
        FDefenseTypeAffixRestriction Row;
        Row.DefenseType = DefenseType;
        Row.RestrictedAffixGroup = Group;
        Restrictions.Add(Row);
    };

    // Armour: heavy melee archetype — no caster/mana stats, no competing defense types.
    AddRestriction(EDefenseType::Armour, FName("Mana"));
    AddRestriction(EDefenseType::Armour, FName("AttrIntelligence"));
    AddRestriction(EDefenseType::Armour, FName("AttrDexterity"));
    AddRestriction(EDefenseType::Armour, FName("ManaRegen"));
    AddRestriction(EDefenseType::Armour, FName("Barrier"));
    AddRestriction(EDefenseType::Armour, FName("Evasion"));
    AddRestriction(EDefenseType::Armour, FName("CastSpeed"));

    // Evasion: agile/dex archetype — no caster/mana stats, no competing defense types.
    AddRestriction(EDefenseType::Evasion, FName("AttrIntelligence"));
    AddRestriction(EDefenseType::Evasion, FName("AttrStrength"));
    AddRestriction(EDefenseType::Evasion, FName("ManaRegen"));
    AddRestriction(EDefenseType::Evasion, FName("Mana"));
    AddRestriction(EDefenseType::Evasion, FName("Barrier"));
    AddRestriction(EDefenseType::Evasion, FName("Armour"));
    AddRestriction(EDefenseType::Evasion, FName("CastSpeed"));

    // Barrier: caster/int archetype — no melee attributes, no competing defense types.
    AddRestriction(EDefenseType::Barrier, FName("AttrDexterity"));
    AddRestriction(EDefenseType::Barrier, FName("AttrStrength"));
    AddRestriction(EDefenseType::Barrier, FName("Evasion"));
    AddRestriction(EDefenseType::Barrier, FName("Armour"));
    AddRestriction(EDefenseType::Barrier, FName("AttackSpeed"));

    return Restrictions;
}

bool UGearBaseFunctionLibrary::IsAffixGroupRestrictedForDefenseType(const TArray<FDefenseTypeAffixRestriction>& Restrictions,
                                                                     FName AffixGroup, EDefenseType DefenseType)
{
    for (const FDefenseTypeAffixRestriction& Row : Restrictions)
    {
        if (Row.DefenseType == DefenseType && Row.RestrictedAffixGroup == AffixGroup)
        {
            return true;
        }
    }
    return false;
}

TArray<FAffixDefinition> UGearBaseFunctionLibrary::FilterAffixPoolForDefenseType(const TArray<FAffixDefinition>& AffixPool,
                                                                                  const TArray<FDefenseTypeAffixRestriction>& Restrictions,
                                                                                  EDefenseType DefenseType)
{
    if (Restrictions.Num() == 0)
    {
        return AffixPool; // nothing restricted — copy the pool through unchanged
    }

    TArray<FAffixDefinition> Filtered;
    Filtered.Reserve(AffixPool.Num());
    for (const FAffixDefinition& Def : AffixPool)
    {
        if (!IsAffixGroupRestrictedForDefenseType(Restrictions, Def.AffixGroup, DefenseType))
        {
            Filtered.Add(Def);
        }
    }
    return Filtered;
}

// ---------------------------------------------------------------------------
// Base + affix integration
// ---------------------------------------------------------------------------

FGearItem UGearBaseFunctionLibrary::GenerateGearItemFromBase(const FGearBaseItem& Base, const TArray<FAffixDefinition>& AffixPool,
                                                               const TArray<FDefenseTypeAffixRestriction>& Restrictions,
                                                               EItemRarity Rarity, float RareFullAffixChance)
{
    const TArray<FAffixDefinition>& EffectivePool = Base.bHasDefenseType
        ? FilterAffixPoolForDefenseType(AffixPool, Restrictions, Base.DefenseType)
        : AffixPool; // Quivers carry no defense family — nothing to restrict against.

    FGearItem Item = UGearAffixFunctionLibrary::GenerateGearItem(Base.BaseId, Base.BaseName, Base.Slot, Base.BaseItemLevel,
                                                                   Rarity, EffectivePool, RareFullAffixChance);
    Item.BaseItemId = Base.BaseId;
    return Item;
}

FGearStatContribution UGearBaseFunctionLibrary::BuildModifierPoolFromGearWithBase(const FGearItem& Item, const FGearBaseItem& Base,
                                                                                    const TArray<FAffixDefinition>& AffixPool)
{
    FGearStatContribution Contribution = UGearAffixFunctionLibrary::BuildModifierPoolFromGear(Item, AffixPool);

    if (Base.bHasDefenseType && Base.BaseDefenseValue != 0.0f)
    {
        Contribution.StatPool.AddFlat(GetStatTypeForDefenseType(Base.DefenseType), Base.BaseDefenseValue);
    }
    if (Base.BaseBlockChance != 0.0f)
    {
        Contribution.StatPool.AddFlat(EStatType::BlockChance, Base.BaseBlockChance);
    }

    return Contribution;
}