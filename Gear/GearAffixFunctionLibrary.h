#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Gear/GearAffixTypes.h"
#include "GearAffixFunctionLibrary.generated.h"

UCLASS()
class UGearAffixFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // How many prefixes/suffixes an item of a given rarity rolls. Normal
    // items always roll 0 (crafting bases only). Magic items always roll
    // exactly 1 prefix + 1 suffix. Rare items roll 2 prefixes + 2 suffixes
    // by default, with a chance (see GenerateGearItem's RareFullAffixChance)
    // to hit the full 3 + 3 instead. Legendary items always roll the full
    // 3 + 3.
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static void GetAffixCountRangeForRarity(EItemRarity Rarity, int32& OutMinPrefixes, int32& OutMaxPrefixes,
                                             int32& OutMinSuffixes, int32& OutMaxSuffixes);

    // Filters AffixPool down to affixes valid for this slot/type that have
    // at least one tier unlocked at ItemLevel. Returns INDICES into AffixPool
    // rather than copies — each FAffixDefinition carries two nested arrays
    // (AllowedSlots-turned-mask aside, Tiers is still an array), so copying
    // full structs on every roll would deep-copy that data repeatedly.
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static TArray<int32> GetEligibleAffixes(const TArray<FAffixDefinition>& AffixPool, EAffixType AffixType,
                                             EGearSlot Slot, int32 ItemLevel);

    // Weighted-random pick of a tier index (into Affix.Tiers) among the
    // tiers unlocked at ItemLevel. Returns -1 if none are unlocked.
    // Allocation-free: walks Affix.Tiers directly with a single-pass
    // weighted reservoir pick instead of building an intermediate weights array.
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static int32 RollTierIndexForAffix(const FAffixDefinition& Affix, int32 ItemLevel);

    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static float RollAffixValue(const FAffixTier& Tier);

    // Generates a full gear item: picks distinct-group affixes, rolls a
    // tier for each (gated by ItemLevel), rolls a value within that tier.
    // RareFullAffixChance is the odds a Rare item rolls the full 3 prefixes
    // + 3 suffixes instead of the standard 2 + 2 (ignored for other rarities).
    UFUNCTION(BlueprintCallable, Category = "Gear Affixes")
    static FGearItem GenerateGearItem(FName ItemId, const FString& ItemName, EGearSlot Slot, int32 ItemLevel,
                                       EItemRarity Rarity, const TArray<FAffixDefinition>& AffixPool,
                                       float RareFullAffixChance = 0.2f);

    // Converts a rolled item's affixes into a derived-stat pool plus base
    // attribute bonuses, ready to apply to FBaseAttributes and feed into
    // FCharacterStats::Recalculate. AffixPool is needed to look up each
    // rolled affix's target/application from its AffixId — this does a
    // linear scan per affix, fine for occasional calls (equip, tooltip). For
    // a large pool called every frame, build an index once with
    // BuildAffixIndex() and use BuildModifierPoolFromGearIndexed instead.
    UFUNCTION(BlueprintCallable, Category = "Gear Affixes")
    static FGearStatContribution BuildModifierPoolFromGear(const FGearItem& Item, const TArray<FAffixDefinition>& AffixPool);

    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static FString DescribeAffix(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool);

    // Builds AffixId -> index-into-AffixPool once. Build this a single time
    // when an affix pool is loaded (e.g. after reading a UDataTable), then
    // reuse it across every BuildModifierPoolFromGearIndexed /
    // DescribeAffixIndexed call instead of re-scanning the pool per lookup.
    UFUNCTION(BlueprintCallable, Category = "Gear Affixes")
    static TMap<FName, int32> BuildAffixIndex(const TArray<FAffixDefinition>& AffixPool);

    // O(1)-per-affix version of BuildModifierPoolFromGear, using a
    // pre-built AffixIndex (see BuildAffixIndex) instead of scanning AffixPool.
    UFUNCTION(BlueprintCallable, Category = "Gear Affixes")
    static FGearStatContribution BuildModifierPoolFromGearIndexed(const FGearItem& Item, const TArray<FAffixDefinition>& AffixPool,
                                                                    const TMap<FName, int32>& AffixIndex);

    // O(1) version of DescribeAffix using a pre-built AffixIndex.
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static FString DescribeAffixIndexed(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool,
                                         const TMap<FName, int32>& AffixIndex);

    // Builds a 9-entry tier array (Tier 9 down to Tier 1) by linearly
    // interpolating roll range, item level requirement, and spawn weight
    // between the Tier-9 and Tier-1 endpoints. Lets an affix be authored
    // with 8 numbers instead of 9 hand-written tier rows.
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static TArray<FAffixTier> BuildTierProgression(float Tier9Min, float Tier9Max, float Tier1Min, float Tier1Max,
                                                     int32 Tier9ItemLevel, int32 Tier1ItemLevel,
                                                     float Tier9Weight, float Tier1Weight);

    // ORs together the slot bits for every slot passed in — use this to
    // build an AllowedSlotsMask from a designer-friendly TArray<EGearSlot>
    // (e.g. in Blueprint, or in C++ authoring code like GetExampleAffixTable).
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static int32 MakeSlotMask(const TArray<EGearSlot>& Slots);

    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static int32 GetAllGearSlotsMask();

    // All gear slots except Weapon — used by the many affixes tagged
    // "Everything but Weapons" (Life, Armour, resistances, etc.).
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static int32 GetNonWeaponGearSlotsMask();

    // Illustrative seed data: an affix definition for every EStatType that
    // carries a Prefix/Suffix designation, plus Strength/Dexterity/
    // Intelligence attribute affixes, following the Added Physical Damage
    // progression (T9: 1-4, T1: 100-200) as a template for authoring the
    // full pool as a UDataTable. Intentionally excludes DodgeChance (not an
    // affix — derived from Evasion), CooldownReduction and AoeRadius
    // (undetermined design), since those have no Prefix/Suffix designation.
    //
    // NOT BlueprintPure: this deep-copies a 29-entry table (each with a
    // nested 9-tier array) on every call. A pure BP node has no result
    // caching, so wiring this into anything that re-evaluates per-tick would
    // rebuild and copy the whole table every frame. Call it once and cache
    // the result (or load the real pool from a UDataTable instead).
    UFUNCTION(BlueprintCallable, Category = "Gear Affixes")
    static TArray<FAffixDefinition> GetExampleAffixTable();

private:
    static const FAffixDefinition* FindDefinition(const TArray<FAffixDefinition>& AffixPool, FName AffixId);
    static const FAffixDefinition* FindDefinitionIndexed(const TArray<FAffixDefinition>& AffixPool,
                                                           const TMap<FName, int32>& AffixIndex, FName AffixId);

    // Single-pass weighted reservoir pick: calls GetWeight(i) for i in
    // [0, Count), treating a weight <= 0 as ineligible, and returns the
    // chosen index (or -1 if every weight was <= 0). No intermediate
    // array is allocated, unlike building a weights array and walking a
    // cumulative sum separately.
    template <typename TWeightFunc>
    static int32 WeightedReservoirPick(int32 Count, TWeightFunc&& GetWeight)
    {
        float TotalWeight = 0.0f;
        int32 Picked = -1;
        for (int32 i = 0; i < Count; ++i)
        {
            const float W = GetWeight(i);
            if (W <= 0.0f)
            {
                continue;
            }
            TotalWeight += W;
            if (FMath::FRand() * TotalWeight <= W)
            {
                Picked = i;
            }
        }
        return Picked;
    }
};