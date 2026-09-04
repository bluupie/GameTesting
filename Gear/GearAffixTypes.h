#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BaseCharacterStats.h"   // EStatType, FModifierPool, FStatModifier
#include "GearAffixTypes.generated.h"

// ---------------------------------------------------------------------------
// Affix Type / Slot / Rarity
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EAffixType : uint8
{
    Prefix,
    Suffix
};

// Whether an affix modifies a derived FCharacterStats value (via
// FModifierPool / EStatType) or a base FBaseAttributes value directly
// (Strength/Dexterity/Intelligence aren't part of EStatType — they're
// inputs to the Recalculate formulas, not outputs of them).
UENUM(BlueprintType)
enum class EAffixTargetType : uint8
{
    Stat,
    Attribute
};

UENUM(BlueprintType)
enum class EBaseAttributeType : uint8
{
    Strength,
    Dexterity,
    Intelligence
};

UENUM(BlueprintType, meta = (Bitmask))
enum class EGearSlot : uint8
{
    Weapon,
    Shield,
    Quiver,
    Helmet,
    Chest,
    Gloves,
    Boots,
    Belt,
    Amulet,
    Ring
};

// Bit for this slot within an AllowedSlotsMask. Stored/exposed as int32
// (not uint32 — Blueprint has no native unsigned-32 type, so UHT won't
// expose a uint32 UPROPERTY/UFUNCTION to BP). EGearSlot has 10 members, so
// this comfortably fits within int32's 31 usable bits with room to grow.
FORCEINLINE int32 GetGearSlotBit(EGearSlot Slot)
{
    return 1 << static_cast<uint8>(Slot);
}

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Normal,     // 0 affixes — crafting base
    Magic,      // always exactly 1 prefix + 1 suffix
    Rare,       // 2 prefixes + 2 suffixes by default, chance to hit 3 + 3
    Legendary   // always exactly 3 prefixes + 3 suffixes
};

// How a rolled affix value is fed into FModifierPool. Mirrors FStatModifier's
// Flat / Percent / MorePercent fields on the existing character stats system.
UENUM(BlueprintType)
enum class EModifierApplication : uint8
{
    Flat,       // FModifierPool::AddFlat        e.g. +37 to Maximum Life
    Increased,  // FModifierPool::AddIncreased   e.g. +24% increased Physical Damage (additive with other Increased)
    More        // FModifierPool::AddMore        e.g. 15% more Damage (multiplicative, rare/special affixes only)
};

// ---------------------------------------------------------------------------
// Affix Tier
// ---------------------------------------------------------------------------

// One tier's roll window for a given affix. TierNumber 1 = best (highest roll,
// highest item level requirement), TierNumber 9 = worst (lowest roll, no
// item level requirement). Example: Added Physical Damage — Tier 9 rolls
// 1-4, Tier 1 rolls 100-200.
USTRUCT(BlueprintType)
struct FAffixTier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 TierNumber = 9;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float MinRoll = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float MaxRoll = 0.0f;

    // Minimum item level required for this tier to be eligible to roll at all.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 RequiredItemLevel = 1;

    // Relative chance this tier is chosen among the tiers currently unlocked
    // for the item's level. Higher-value (better) tiers should use lower
    // weights so they're rarer — see the example table in the function
    // library for a worked progression.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float Weight = 1.0f;
};

// ---------------------------------------------------------------------------
// Affix Definition (data-table row: one row per affix)
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FAffixDefinition : public FTableRowBase
{
    GENERATED_BODY()

    // Unique identifier, e.g. "Prefix_AddedPhysicalDamage". Used to look the
    // definition back up from a rolled FRolledAffix stored on an item.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName AffixId;

    // Printf-style format string for display, e.g. "+%.0f to Maximum Life"
    // or "%.0f%% increased Attack Speed" (value is pre-multiplied by 100
    // by the caller for percent-style affixes if desired).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FString DisplayFormat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EAffixType AffixType = EAffixType::Prefix;

    // Whether this affix modifies a derived stat (StatType/ModApplication
    // apply) or a base attribute (AttributeType applies instead, and the
    // roll is always treated as a flat addition — see BuildModifierPoolFromGear).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EAffixTargetType TargetType = EAffixTargetType::Stat;

    // Only used when TargetType == Stat.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EStatType StatType = EStatType::Life;

    // Only used when TargetType == Stat. Applied to StatType AND every
    // entry in AdditionalStatTypes below, using the same ModApplication and
    // the same rolled value for all of them — this is what lets one affix
    // (one roll, one tier) act as a multi-stat modifier, e.g. "increased
    // Elemental Damage" boosting DamageFire + DamageCold + DamageLightning
    // simultaneously rather than needing three separate affixes. Leave
    // empty for an ordinary single-stat affix.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    TArray<EStatType> AdditionalStatTypes;

    // Only used when TargetType == Stat.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EModifierApplication ModApplication = EModifierApplication::Flat;

    // Only used when TargetType == Attribute.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EBaseAttributeType AttributeType = EBaseAttributeType::Strength;

    // If true, RolledValue is a fraction (0.15 = 15%) that should be
    // multiplied by 100 before being substituted into DisplayFormat's
    // "%.0f%%"-style placeholder. Leave false for non-percent values
    // (flat Life, Armour, Spell Damage, etc.).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    bool bIsPercentageValue = false;

    // Mutual-exclusion tag. Two affixes sharing a group will never both be
    // rolled onto the same item (e.g. flat Life and % increased Life both
    // tagged "Life" so an item can't roll both).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName AffixGroup;

    // Bitmask of eligible EGearSlot values, built with GetGearSlotBit() /
    // UGearAffixFunctionLibrary::MakeSlotMask(). int32 (not uint32 — see
    // GetGearSlotBit's comment) with the Bitmask meta so the editor renders
    // it as a checkbox dropdown against EGearSlot instead of a raw integer.
    // Replaces a TArray<EGearSlot> so eligibility checks and the affix pool
    // itself avoid a per-affix heap allocation and a linear Contains() scan
    // — this runs on every item roll.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix", meta = (Bitmask, BitmaskEnum = "EGearSlot"))
    int32 AllowedSlotsMask = 0;

    // Expected to contain 9 entries with TierNumber 1..9, but the system
    // does not hard-require exactly 9 — HasUnlockedTier/GetUnlockedTierIndices
    // just filter by RequiredItemLevel, so partially-defined affixes still work.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    TArray<FAffixTier> Tiers;

    // Relative chance this affix is picked over other eligible affixes of
    // the same AffixType when generating an item.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float SpawnWeight = 100.0f;

    bool IsEligibleForSlot(EGearSlot Slot) const
    {
        return (AllowedSlotsMask & GetGearSlotBit(Slot)) != 0;
    }

    // Non-allocating existence check — use this on the roll hot path instead
    // of GetUnlockedTierIndices(...).Num() > 0, which builds and discards a
    // whole TArray just to answer a bool.
    bool HasUnlockedTier(int32 ItemLevel) const
    {
        for (const FAffixTier& Tier : Tiers)
        {
            if (Tier.RequiredItemLevel <= ItemLevel)
            {
                return true;
            }
        }
        return false;
    }

    // Returns indices into Tiers whose RequiredItemLevel <= ItemLevel.
    // Allocates — kept for tooling/UI/debug use (e.g. listing all tiers an
    // item level could roll). Not used on the roll hot path; see
    // UGearAffixFunctionLibrary::RollTierIndexForAffix for the allocation-free
    // version used there.
    TArray<int32> GetUnlockedTierIndices(int32 ItemLevel) const
    {
        TArray<int32> Result;
        for (int32 i = 0; i < Tiers.Num(); ++i)
        {
            if (Tiers[i].RequiredItemLevel <= ItemLevel)
            {
                Result.Add(i);
            }
        }
        return Result;
    }
};

// ---------------------------------------------------------------------------
// Rolled Affix Instance (what actually lives on an item)
// ---------------------------------------------------------------------------

// Deliberately minimal — everything else (stat, application type, display
// format) is looked up from the FAffixDefinition via AffixId so items don't
// duplicate authoring data. Only the roll outcome is stored per-item.
USTRUCT(BlueprintType)
struct FRolledAffix
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName AffixId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 TierNumber = 9;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float RolledValue = 0.0f;
};

// ---------------------------------------------------------------------------
// Gear Item
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FGearItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EGearSlot GearSlot = EGearSlot::Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemRarity Rarity = EItemRarity::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 ItemLevel = 1;

    // Which FGearBaseItem::BaseId this item was generated from, if any
    // (see UGearBaseFunctionLibrary::GenerateGearItemFromBase). Lets the
    // base's own innate defense value be looked back up alongside the
    // rolled affixes — see BuildModifierPoolFromGearWithBase. Empty for
    // items built directly via GenerateGearItem without a base.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName BaseItemId;

    // Max 3 — enforced by the generator, not the struct itself.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TArray<FRolledAffix> PrefixAffixes;

    // Max 3 — enforced by the generator, not the struct itself.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TArray<FRolledAffix> SuffixAffixes;

    int32 GetTotalAffixCount() const
    {
        return PrefixAffixes.Num() + SuffixAffixes.Num();
    }
};

// ---------------------------------------------------------------------------
// Gear Stat Contribution
// ---------------------------------------------------------------------------

// Output of converting a gear item's rolled affixes into something usable by
// the character stats system. StatPool feeds FCharacterStats::Recalculate
// directly; the Bonus* attribute fields should be added onto FBaseAttributes
// BEFORE calling Recalculate, since Strength/Dexterity/Intelligence are
// inputs to those formulas rather than outputs of them.
USTRUCT(BlueprintType)
struct FGearStatContribution
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Item")
    FModifierPool StatPool;

    UPROPERTY(BlueprintReadOnly, Category = "Item")
    float BonusStrength = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Item")
    float BonusDexterity = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Item")
    float BonusIntelligence = 0.0f;
};