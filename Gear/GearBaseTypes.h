#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Gear/GearAffixTypes.h"   // EGearSlot, EStatType (via BaseCharacterStats.h)
#include "Gear/GearBaseTypes.generated.h"

// ---------------------------------------------------------------------------
// Defense Type / Off-Hand Type
// ---------------------------------------------------------------------------

// The three parallel defensive stats an armour piece (or shield) is built
// around. Deliberately a separate enum from EStatType rather than reusing
// EStatType::Armour/Evasion/Barrier directly, so "which defense family is
// this base" reads as a first-class concept on FGearBaseItem instead of
// overloading EStatType's meaning — see GetStatTypeForDefenseType() for
// where the two connect.
UENUM(BlueprintType)
enum class EDefenseType : uint8
{
    Armour,
    Evasion,
    Barrier
};

// Maps a defense family onto the EStatType its innate BaseDefenseValue is
// applied to (as a flat contribution, before affixes).
FORCEINLINE EStatType GetStatTypeForDefenseType(EDefenseType DefenseType)
{
    switch (DefenseType)
    {
        case EDefenseType::Armour:  return EStatType::Armour;
        case EDefenseType::Evasion: return EStatType::Evasion;
        case EDefenseType::Barrier: return EStatType::Barrier;
        default:                    return EStatType::Armour;
    }
}

UENUM(BlueprintType)
enum class EOffHandType : uint8
{
    Shield,
    Quiver
};

// ---------------------------------------------------------------------------
// Base Level Progression
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FGearBaseLevelProgression
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Gear Base")
    int32 RequiredCharacterLevel = 1;

    // Feeds UGearAffixFunctionLibrary::GenerateGearItem's ItemLevel param —
    // this is what gates which affix tiers can roll on the item
    // (FAffixTier::RequiredItemLevel), independently of RequiredCharacterLevel.
    UPROPERTY(BlueprintReadOnly, Category = "Gear Base")
    int32 BaseItemLevel = 1;
};

// ---------------------------------------------------------------------------
// Gear Base Item (armour piece / shield / quiver template)
// ---------------------------------------------------------------------------

// A "base" is the unaffixed template an actual item is generated from — e.g.
// "Dragon Platebody" is a base; a specific Rare Dragon Platebody with rolled
// affixes is an FGearItem referencing it via BaseItemId. Inherits
// FTableRowBase so the real base pool can be authored as a UDataTable,
// same pattern as FAffixDefinition.
USTRUCT(BlueprintType)
struct FGearBaseItem : public FTableRowBase
{
    GENERATED_BODY()

    // Unique identifier, e.g. "Armour_Chest_Rank5" or "Shield_Barrier_Rank3".
    // Stored on FGearItem::BaseItemId once an item is generated from this base.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    FName BaseId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    FString BaseName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    EGearSlot Slot = EGearSlot::Helmet;

    // Meaningful for Helmet/Chest/Gloves/Boots and OffHand Shields. Ignored
    // for OffHand Quivers — see bHasDefenseType.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    EDefenseType DefenseType = EDefenseType::Armour;

    // False for Quivers (no defense family) so callers don't mistakenly
    // apply BaseDefenseValue to a stat that isn't meant to exist on them.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    bool bHasDefenseType = true;

    // Only meaningful when Slot == OffHand.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    EOffHandType OffHandType = EOffHandType::Shield;

    // Character level required to equip this base.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    int32 RequiredCharacterLevel = 1;

    // Drives which affix tiers an item generated from this base can roll —
    // see FAffixTier::RequiredItemLevel and
    // UGearAffixFunctionLibrary::GetEligibleAffixes/RollTierIndexForAffix.
    // Deliberately tracked separately from RequiredCharacterLevel: a base's
    // item level (roughly "how good a drop it is") and the character level
    // needed to wear it are related but not forced to match 1:1.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    int32 BaseItemLevel = 1;

    // Innate flat defense value granted by the base itself, applied to
    // GetStatTypeForDefenseType(DefenseType) BEFORE affixes are added.
    // 0 / unused for Quivers (bHasDefenseType == false).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    float BaseDefenseValue = 0.0f;

    // Innate Block Chance (fraction — 0.15 = 15%). Only Shields carry a
    // nonzero value here; 0 for every armour slot and for Quivers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear Base")
    float BaseBlockChance = 0.0f;
};