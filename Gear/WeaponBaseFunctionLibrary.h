#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Gear/GearAffixFunctionLibrary.h"
#include "Gear/WeaponBaseTypes.h"
#include "WeaponBaseFunctionLibrary.generated.h"

UCLASS()
class UWeaponBaseFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Weapon Bases")
    static FGearItem GenerateWeaponItemFromBase(const FWeaponBaseItem& Base,
                                                 const TArray<FAffixDefinition>& AffixPool,
                                                 EItemRarity Rarity,
                                                 float RareFullAffixChance = 0.2f,
                                                 int32 RandomSeed = 0);

    // Converts a main-hand weapon into the central stat representation. The
    // eventual tagged combat pipeline should resolve off-hand/dual-wield local
    // damage independently rather than adding a second weapon base globally.
    UFUNCTION(BlueprintCallable, Category = "Weapon Bases")
    static FGearStatContribution BuildModifierPoolFromGearWithWeaponBase(
        const FGearItem& Item,
        const FWeaponBaseItem& Base,
        const TArray<FAffixDefinition>& AffixPool);
};
