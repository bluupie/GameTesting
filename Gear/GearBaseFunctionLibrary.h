#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Gear/GearAffixFunctionLibrary.h"
#include "Gear/GearBaseTypes.h"
#include "Gear/GearBaseFunctionLibrary.generated.h"

UCLASS()
class UGearBaseFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Gear Bases")
    static FGearBaseLevelProgression GetBaseLevelProgression(int32 Rank, int32 TptalRanks, int32 MinCharacterLevel = 1, int32 MaxCharacterLevel = 80, int32 MinItemLevel = 1, MaxItemLevel = 90)

    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetExampleArmourBaseTable();

    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetExampleShieldBaseTable();

    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetExampleQuiverBaseTable();

    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetExampleGearBaseTable();

    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static FGearItem GenerateGearItemFromBase(const FGearBaseItem& Base, const TArray<FAffixDefinition>& AffixPool, EItemRarity Rarity, float RareFullAffixChance = 0.2f);

    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static FGearStatContribution BuildModifierPoolFromGearWithBase(const FGearItem& Item, const FGearBaseItem& Base, const TArray<FAffixDefinition>& AffixPool);

    private:
        static FString BuildArmourBaseName(EDefenseType DefenseType, EGearSlot Slot, int32 Rank);
        static FString BuildShieldBaseName(EDefenseType DefenseType, EGearSlot Slot, int32 Rank);
        static float LerpRankValue(int32 Rank, int32 TotalRanks, flot MinValue, float MaxValue);
};