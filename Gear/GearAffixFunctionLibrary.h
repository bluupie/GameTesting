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
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static TMap<FName, int32> BuildAffixIndex(const TArray<FAffixDefinition>& AffixPool);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static void GetAffixCountRangeForRarity(EItemRarity Rarity, int32& OutMinPrefixes, int32& OutMaxPrefixes,
                                             int32& OutMinSuffixes, int32& OutMaxSuffixes);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static TArray<int32> GetAffixesWithTag(const TArray<FAffixDefinition>& AffixPool, FName Tag);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static TArray<int32> GetAffixesWithAnyTag(const TArray<FAffixDefinition>& AffixPool, const TArray<FName>& Tags);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static TArray<int32> GetEligibleAffixes(const TArray<FAffixDefinition>& AffixPool, EAffixType AffixType,
                                            EGearSlot Slot, int32 ItemLevel);
    UFUNCTION(BlueprintCallable, Category = "Gear|Affixes")
    static int32 RollTierIndexForAffix(const FAffixDefinition& Affix, int32 ItemLevel);
    UFUNCTION(BlueprintCallable, Category = "Gear|Affixes")
    static float RollAffixValue(const FAffixTier& Tier);
    UFUNCTION(BlueprintCallable, Category = "Gear|Affixes")
    static FGearItem GenerateGearItem(FName BaseItemId, const FString& ItemName, EGearSlot Slot, int32 ItemLevel,
                                      EItemRarity Rarity, const TArray<FAffixDefinition>& AffixPool,
                                      float RareFullAffixChance = 0.2f, int32 RandomSeed = 0);
    UFUNCTION(BlueprintCallable, Category = "Gear|Affixes")
    static FGearStatContribution BuildModifierPoolFromGear(const FGearItem& Item, const TArray<FAffixDefinition>& AffixPool);
    UFUNCTION(BlueprintCallable, Category = "Gear|Affixes")
    static FGearStatContribution BuildModifierPoolFromGearIndexed(const FGearItem& Item,
                                                                   const TArray<FAffixDefinition>& AffixPool,
                                                                   const TMap<FName, int32>& AffixIndex);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static FString DescribeAffix(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static FString DescribeAffixIndexed(const FRolledAffix& Rolled, const TArray<FAffixDefinition>& AffixPool,
                                         const TMap<FName, int32>& AffixIndex);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static TArray<FAffixTier> BuildTierProgression(float Tier9Min, float Tier9Max, float Tier1Min, float Tier1Max,
                                                   int32 Tier9ItemLevel = 1, int32 Tier1ItemLevel = 85,
                                                   float Tier9Weight = 100.0f, float Tier1Weight = 5.0f);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static int32 MakeSlotMask(const TArray<EGearSlot>& Slots);
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static int32 GetAllGearSlotsMask();
    UFUNCTION(BlueprintPure, Category = "Gear|Affixes")
    static int32 GetNonWeaponGearSlotsMask();
    UFUNCTION(BlueprintCallable, Category = "Gear|Affixes")
    static TArray<FAffixDefinition> GetExampleAffixTable();

private:
    static const FAffixDefinition* FindDefinition(const TArray<FAffixDefinition>& AffixPool, FName AffixId);
    static const FAffixDefinition* FindDefinitionIndexed(const TArray<FAffixDefinition>& AffixPool,
                                                          const TMap<FName, int32>& AffixIndex, FName AffixId);

    template <typename WeightGetter>
    static int32 WeightedReservoirPick(int32 Count, WeightGetter&& GetWeight)
    {
        int32 ChosenIndex = INDEX_NONE;
        float TotalWeight = 0.0f;
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float Weight = FMath::Max(0.0f, GetWeight(Index));
            if (Weight <= 0.0f) continue;
            TotalWeight += Weight;
            if (FMath::FRand() * TotalWeight < Weight) ChosenIndex = Index;
        }
        return ChosenIndex;
    }
};
