#include "WeaponBaseFunctionLibrary.h"

FGearItem UWeaponBaseFunctionLibrary::GenerateWeaponItemFromBase(
    const FWeaponBaseItem& Base,
    const TArray<FAffixDefinition>& AffixPool,
    EItemRarity Rarity,
    float RareFullAffixChance,
    int32 RandomSeed)
{
    FGearItem Item = UGearAffixFunctionLibrary::GenerateGearItem(
        Base.BaseId,
        Base.BaseName,
        EGearSlot::Weapon,
        Base.BaseItemLevel,
        Rarity,
        AffixPool,
        RareFullAffixChance,
        RandomSeed);

    Item.BaseItemId = Base.BaseId;
    Item.RequiredCharacterLevel = Base.RequiredCharacterLevel;
    Item.RequiredStrength = Base.RequiredStrength;
    Item.RequiredDexterity = Base.RequiredDexterity;
    Item.RequiredIntelligence = Base.RequiredIntelligence;
    return Item;
}

FGearStatContribution UWeaponBaseFunctionLibrary::BuildModifierPoolFromGearWithWeaponBase(
    const FGearItem& Item,
    const FWeaponBaseItem& Base,
    const TArray<FAffixDefinition>& AffixPool)
{
    FGearStatContribution Contribution =
        UGearAffixFunctionLibrary::BuildModifierPoolFromGear(Item, AffixPool);

    const float AveragePhysicalDamage =
        (FMath::Max(0.0f, Base.BaseMinPhysicalDamage)
         + FMath::Max(0.0f, Base.BaseMaxPhysicalDamage)) * 0.5f;

    Contribution.StatPool.AddFlat(EStatType::DamagePhysical, AveragePhysicalDamage);
    Contribution.StatPool.AddFlat(EStatType::AttackSpeed, Base.BaseAttackSpeed - 1.0f);
    Contribution.StatPool.AddFlat(EStatType::CriticalStrikeChance,
                                  Base.BaseCriticalStrikeChance - 0.05f);
    return Contribution;
}
