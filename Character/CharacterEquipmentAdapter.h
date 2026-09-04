#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterEquipmentFunctionLibrary.h"
#include "CharacterStatsComponent.h"
#include "Gear/GearBaseFunctionLibrary.h"
#include "Gear/WeaponBaseFunctionLibrary.h"
#include "CharacterEquipmentAdapter.generated.h"

USTRUCT(BlueprintType)
struct FEquipmentStatDefinitionSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Definitions")
    TArray<FAffixDefinition> AffixDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Definitions")
    TArray<FGearBaseItem> GearBases;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Definitions")
    TArray<FWeaponBaseItem> WeaponBases;
};

UENUM(BlueprintType)
enum class EEquipmentAdapterResult : uint8
{
    Success,
    Failed_StatsComponentUnavailable,
    Failed_NotAuthority,
    Failed_ItemNotFound,
    Failed_InvalidItemId,
    Failed_DuplicateItemId,
    Failed_ItemAlreadyEquipped,
    Failed_InventoryFull,
    Failed_EquipRules,
    Failed_StatRefresh
};

USTRUCT(BlueprintType)
struct FEquipmentAdapterOutcome
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    EEquipmentAdapterResult Result = EEquipmentAdapterResult::Failed_EquipRules;

    // Contains the detailed slot/level/attribute/weapon compatibility result.
    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    EEquipResult EquipResult = EEquipResult::Success;

    // Items returned to inventory by a successful replacement or unequip.
    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    TArray<FGearItem> DisplacedItems;
};

UCLASS()
class UCharacterEquipmentAdapter : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Equipment|Integration")
    static bool RefreshEquipmentStats(UCharacterStatsComponent* StatsComponent,
                                      const FCharacterEquipment& Equipment,
                                      const FEquipmentStatDefinitionSet& Definitions,
                                      EResourceRecalculationPolicy ResourcePolicy =
                                          EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Integration")
    static FEquipmentAdapterOutcome EquipItemFromInventory(
        UCharacterStatsComponent* StatsComponent,
        UPARAM(ref) FCharacterEquipment& Equipment,
        UPARAM(ref) TArray<FGearItem>& InventoryItems,
        int32 InventoryCapacity,
        EEquipmentSlot Slot,
        FName ItemId,
        const FEquipmentStatDefinitionSet& Definitions);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Integration")
    static FEquipmentAdapterOutcome EquipMainHandFromInventory(
        UCharacterStatsComponent* StatsComponent,
        UPARAM(ref) FCharacterEquipment& Equipment,
        UPARAM(ref) TArray<FGearItem>& InventoryItems,
        int32 InventoryCapacity,
        FName ItemId,
        EWeaponCategory WeaponCategory,
        const FEquipmentStatDefinitionSet& Definitions);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Integration")
    static FEquipmentAdapterOutcome EquipOffHandFromInventory(
        UCharacterStatsComponent* StatsComponent,
        UPARAM(ref) FCharacterEquipment& Equipment,
        UPARAM(ref) TArray<FGearItem>& InventoryItems,
        int32 InventoryCapacity,
        FName ItemId,
        EOffHandItemKind ItemKind,
        EWeaponCategory OffHandWeaponCategory,
        const FEquipmentStatDefinitionSet& Definitions);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Integration")
    static FEquipmentAdapterOutcome UnequipToInventory(
        UCharacterStatsComponent* StatsComponent,
        UPARAM(ref) FCharacterEquipment& Equipment,
        UPARAM(ref) TArray<FGearItem>& InventoryItems,
        int32 InventoryCapacity,
        EEquipmentSlot Slot,
        const FEquipmentStatDefinitionSet& Definitions);

private:
    static int32 FindInventoryItemIndex(const TArray<FGearItem>& InventoryItems, FName ItemId);
    static bool IsItemAlreadyEquipped(const FCharacterEquipment& Equipment, FName ItemId);
    static bool HasValidUniqueItemIds(const TArray<FGearItem>& InventoryItems,
                                      const FCharacterEquipment& Equipment);
    static FGearStatContribution BuildContribution(
        EEquipmentSlot EquipmentSlot,
        const FGearItem& Item,
        const FEquipmentStatDefinitionSet& Definitions);
    static bool BuildStatSources(
        const FCharacterEquipment& Equipment,
        const FEquipmentStatDefinitionSet& Definitions,
        TArray<FCharacterStatSource>& OutSources);
    static FEquipmentAdapterOutcome CommitTransaction(
        UCharacterStatsComponent* StatsComponent,
        FCharacterEquipment& Equipment,
        TArray<FGearItem>& InventoryItems,
        int32 InventoryCapacity,
        const FCharacterEquipment& PreparedEquipment,
        int32 InventoryItemIndexToRemove,
        const TArray<FGearItem>& ItemsToReturn,
        const FEquipmentStatDefinitionSet& Definitions);
};
