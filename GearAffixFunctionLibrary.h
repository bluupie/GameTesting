#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GearAffixTypes.h"
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
    // at least one tier unlocked at ItemLevel.
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static TArray<FAffixDefinition> GetEligibleAffixes(const TArray<FAffixDefinition>& AffixPool, EAffixType AffixType,
                                                        EGearSlot Slot, int32 ItemLevel);

    // Weighted-random pick of a tier index (into Affix.Tiers) among the
    // tiers unlocked at ItemLevel. Returns -1 if none are unlocked.
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

    // Converts a rolled item's affixes into an FModifierPool ready to feed
    // into FCharacterStats::Recalculate. AffixPool is needed to look up each
    // rolled affix's StatType/ModApplication from its AffixId.
    UFUNCTION(BlueprintCallable, Category = "Gear Affixes")
    static FModifierPool BuildModifierPoolFromGear(const FGearItem& Item, const TArray<FAffixDefinition>& AffixPool);

    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static FString DescribeAffix(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool);

    // Illustrative seed data: 6 affixes (2 weapon prefixes, 1 weapon suffix,
    // 2 armor suffixes) each fully populated across 9 tiers, following the
    // Added Physical Damage progression (T9: 1-4, T1: 100-200) as a template
    // for authoring the rest of the affix pool as a UDataTable.
    UFUNCTION(BlueprintPure, Category = "Gear Affixes")
    static TArray<FAffixDefinition> GetExampleAffixTable();

private:
    static const FAffixDefinition* FindDefinition(const TArray<FAffixDefinition>& AffixPool, FName AffixId);
    static int32 PickWeightedIndex(const TArray<float>& Weights);
};