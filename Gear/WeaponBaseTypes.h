#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Gear/GearAffixTypes.h"   // EGearSlot, EStatType
#include "WeaponBaseTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
    OneHandedSword,
    TwoHandedSword,
    OneHandedAxe,
    TwoHandedAxe,
    Wand,
    Sceptre,
    Bow
};

// What can occupy OffHand while a weapon of this category sits in MainHand.
UENUM(BlueprintType)
enum class EOffHandCompatibility : uint8
{
    BlockedByTwoHanded, // the weapon itself occupies OffHand — nothing else can
    ShieldOrDualWield,  // a Shield, or another one-handed weapon
    QuiverOnly          // only a Quiver (Bow)
};

FORCEINLINE bool IsTwoHandedWeapon(EWeaponCategory Category)
{
    return Category == EWeaponCategory::TwoHandedSword || Category == EWeaponCategory::TwoHandedAxe;
}

FORCEINLINE EOffHandCompatibility GetOffHandCompatibility(EWeaponCategory Category)
{
    switch (Category)
    {
        case EWeaponCategory::TwoHandedSword:
        case EWeaponCategory::TwoHandedAxe:
            return EOffHandCompatibility::BlockedByTwoHanded;
        case EWeaponCategory::Bow:
            return EOffHandCompatibility::QuiverOnly;
        default: // OneHandedSword, OneHandedAxe, Wand, Sceptre
            return EOffHandCompatibility::ShieldOrDualWield;
    }
}

// A weapon base template — the weapon-side counterpart to FGearBaseItem.
// Carries damage/speed/crit innate stats instead of a defense value.
// Inherits FTableRowBase so the real weapon pool can be authored as a
// UDataTable, same pattern as FAffixDefinition and FGearBaseItem.
USTRUCT(BlueprintType)
struct FWeaponBaseItem : public FTableRowBase
{
    GENERATED_BODY()

    // Unique identifier, e.g. "OneHandedSword_Rank5". Stored on
    // FGearItem::BaseItemId once an item is generated from this base.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    FName BaseId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    FString BaseName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    EWeaponCategory WeaponCategory = EWeaponCategory::OneHandedSword;

    // Character level required to equip this base.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    int32 RequiredCharacterLevel = 1;

    // Drives which affix tiers an item generated from this base can roll —
    // same role as FGearBaseItem::BaseItemLevel.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    int32 BaseItemLevel = 1;

    // Innate physical damage range granted by the base itself, applied as
    // its average to EStatType::DamagePhysical BEFORE affixes — see
    // UWeaponBaseFunctionLibrary::BuildModifierPoolFromGearWithWeaponBase.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    float BaseMinPhysicalDamage = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    float BaseMaxPhysicalDamage = 2.0f;

    // Fixed per weapon CATEGORY, not scaled by rank — a Wand is always
    // faster than a Greataxe regardless of item quality. Multiplier, same
    // convention as FCharacterStats::AttackSpeed (1.0 = baseline).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    float BaseAttackSpeed = 1.0f;

    // Fraction (0.06 = 6%), same convention as FCharacterStats::CriticalStrikeChance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Base")
    float BaseCriticalStrikeChance = 0.05f;
};