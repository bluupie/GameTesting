#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Gear/GearAffixTypes.h"
#include "Gear/WeaponBaseTypes.h"
#include "CharacterEquipmentFunctionLibrary.generated.h"

// The character-facing slots. Ring1 and Ring2 intentionally map to the same
// EGearSlot::Ring item type while remaining separate equipped positions.
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    MainHand,
    OffHand,
    Helmet,
    Chest,
    Gloves,
    Boots,
    Belt,
    Amulet,
    Ring1,
    Ring2
};

// EGearSlot tells us what an item is; this enum tells us how an item occupying
// OffHand participates in the weapon compatibility rules.
UENUM(BlueprintType)
enum class EOffHandItemKind : uint8
{
    Shield,
    Quiver,
    DualWieldWeapon
};

UENUM(BlueprintType)
enum class EEquipResult : uint8
{
    Success,
    Failed_WrongItemSlot,
    Failed_LevelRequirement,
    Failed_UseWeaponEquipFunction,
    Failed_SlotAlreadyEmpty,
    Failed_OffHandBlockedByTwoHanded,
    Failed_IncompatibleOffHand,
    Failed_DualWieldRequiresOneHanded,
    Failed_DualWieldRequiresMatchingType
};

// Persistent equipped state. The two metadata-valid flags prevent the enum
// defaults from being mistaken for real Sword/Shield state after removal.
USTRUCT(BlueprintType)
struct FCharacterEquipment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Equipment")
    TMap<EEquipmentSlot, FGearItem> EquippedItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Equipment|Weapon")
    EWeaponCategory MainHandWeaponCategory = EWeaponCategory::OneHandedSword;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Equipment|Weapon")
    bool bHasMainHandWeaponMetadata = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Equipment|Weapon")
    EOffHandItemKind CurrentOffHandKind = EOffHandItemKind::Shield;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Equipment|Weapon")
    EWeaponCategory CurrentOffHandWeaponCategory = EWeaponCategory::OneHandedSword;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Equipment|Weapon")
    bool bHasOffHandMetadata = false;

    const FGearItem* GetEquippedItem(EEquipmentSlot Slot) const
    {
        return EquippedItems.Find(Slot);
    }

    FGearItem* GetEquippedItem(EEquipmentSlot Slot)
    {
        return EquippedItems.Find(Slot);
    }

    bool IsSlotOccupied(EEquipmentSlot Slot) const
    {
        return EquippedItems.Contains(Slot);
    }

    void SetMainHandMetadata(EWeaponCategory Category)
    {
        MainHandWeaponCategory = Category;
        bHasMainHandWeaponMetadata = true;
    }

    void ClearMainHandMetadata()
    {
        MainHandWeaponCategory = EWeaponCategory::OneHandedSword;
        bHasMainHandWeaponMetadata = false;
    }

    void SetOffHandMetadata(EOffHandItemKind Kind, EWeaponCategory WeaponCategory)
    {
        CurrentOffHandKind = Kind;
        CurrentOffHandWeaponCategory = WeaponCategory;
        bHasOffHandMetadata = true;
    }

    void ClearOffHandMetadata()
    {
        CurrentOffHandKind = EOffHandItemKind::Shield;
        CurrentOffHandWeaponCategory = EWeaponCategory::OneHandedSword;
        bHasOffHandMetadata = false;
    }
};

// Outcome for operations that can displace at most one item.
USTRUCT(BlueprintType)
struct FEquipOutcome
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    EEquipResult Result = EEquipResult::Failed_WrongItemSlot;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    bool bHadPreviousItem = false;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    FGearItem PreviousItem;

    bool Succeeded() const
    {
        return Result == EEquipResult::Success;
    }
};

// Main-hand changes may replace the old weapon and independently invalidate
// the current off-hand item, so both displaced values are returned.
USTRUCT(BlueprintType)
struct FMainHandEquipOutcome
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    EEquipResult Result = EEquipResult::Failed_WrongItemSlot;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    bool bHadPreviousMainHand = false;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    FGearItem PreviousMainHand;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    bool bHadBumpedOffHand = false;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    FGearItem BumpedOffHand;

    bool Succeeded() const
    {
        return Result == EEquipResult::Success;
    }
};

UCLASS()
class UCharacterEquipmentFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Equips armour or jewellery. MainHand and OffHand must use their
    // specialized functions so weapon compatibility cannot be bypassed.
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    static FEquipOutcome EquipItem(UPARAM(ref) FCharacterEquipment& Equipment,
                                   EEquipmentSlot Slot, const FGearItem& Item,
                                   int32 CharacterLevel = 1);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    static FEquipOutcome UnequipSlot(UPARAM(ref) FCharacterEquipment& Equipment,
                                     EEquipmentSlot Slot);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
    static FMainHandEquipOutcome EquipMainHandWeapon(UPARAM(ref) FCharacterEquipment& Equipment,
                                                      const FGearItem& WeaponItem,
                                                      EWeaponCategory WeaponCategory,
                                                      int32 CharacterLevel = 1);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
    static FMainHandEquipOutcome UnequipMainHandWeapon(UPARAM(ref) FCharacterEquipment& Equipment);

    UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon")
    static FEquipOutcome EquipOffHand(UPARAM(ref) FCharacterEquipment& Equipment,
                                      const FGearItem& OffHandItem,
                                      EOffHandItemKind ItemKind,
                                      EWeaponCategory OffHandWeaponCategory,
                                      int32 CharacterLevel = 1);

    UFUNCTION(BlueprintPure, Category = "Equipment")
    static bool MeetsLevelRequirement(const FGearItem& Item, int32 CharacterLevel);

private:
    static EGearSlot GetExpectedGearSlot(EEquipmentSlot Slot);
};
