#include "CharacterEquipmentAdapter.h"

namespace EquipmentAdapterPrivate
{
    void ApplyContribution(FCharacterStatSource& Source, const FGearStatContribution& Contribution)
    {
        Source.SourceType = ECharacterStatSourceType::Equipment;
        Source.StatPool = Contribution.StatPool;
        Source.BonusStrength = FMath::RoundToInt(Contribution.BonusStrength);
        Source.BonusDexterity = FMath::RoundToInt(Contribution.BonusDexterity);
        Source.BonusIntelligence = FMath::RoundToInt(Contribution.BonusIntelligence);
    }

    void AddDisplacedItem(TArray<FGearItem>& Items, bool bHasItem, const FGearItem& Item)
    {
        if (bHasItem)
        {
            Items.Add(Item);
        }
    }
}

int32 UCharacterEquipmentAdapter::FindInventoryItemIndex(
    const TArray<FGearItem>& InventoryItems,
    FName ItemId)
{
    return InventoryItems.IndexOfByPredicate(
        [ItemId](const FGearItem& Item) { return Item.ItemId == ItemId; });
}

bool UCharacterEquipmentAdapter::IsItemAlreadyEquipped(
    const FCharacterEquipment& Equipment,
    FName ItemId)
{
    for (const TPair<EEquipmentSlot, FGearItem>& Pair : Equipment.EquippedItems)
    {
        if (Pair.Value.ItemId == ItemId)
        {
            return true;
        }
    }
    return false;
}

bool UCharacterEquipmentAdapter::HasValidUniqueItemIds(
    const TArray<FGearItem>& InventoryItems,
    const FCharacterEquipment& Equipment)
{
    TSet<FName> SeenIds;
    for (const FGearItem& Item : InventoryItems)
    {
        if (Item.ItemId.IsNone() || SeenIds.Contains(Item.ItemId))
        {
            return false;
        }
        SeenIds.Add(Item.ItemId);
    }

    for (const TPair<EEquipmentSlot, FGearItem>& Pair : Equipment.EquippedItems)
    {
        const FName ItemId = Pair.Value.ItemId;
        if (ItemId.IsNone() || SeenIds.Contains(ItemId))
        {
            return false;
        }
        SeenIds.Add(ItemId);
    }
    return true;
}

FGearStatContribution UCharacterEquipmentAdapter::BuildContribution(
    EEquipmentSlot EquipmentSlot,
    const FGearItem& Item,
    const FEquipmentStatDefinitionSet& Definitions)
{
    // Only MainHand contributes the weapon base globally. Off-hand weapon
    // base damage is intentionally deferred to the tagged dual-wield combat
    // resolver; its global affixes still contribute here.
    if (EquipmentSlot == EEquipmentSlot::MainHand && Item.GearSlot == EGearSlot::Weapon)
    {
        for (const FWeaponBaseItem& Base : Definitions.WeaponBases)
        {
            if (Base.BaseId == Item.BaseItemId)
            {
                return UWeaponBaseFunctionLibrary::BuildModifierPoolFromGearWithWeaponBase(
                    Item, Base, Definitions.AffixDefinitions);
            }
        }
    }
    else if (Item.GearSlot != EGearSlot::Weapon)
    {
        for (const FGearBaseItem& Base : Definitions.GearBases)
        {
            if (Base.BaseId == Item.BaseItemId && Base.Slot == Item.GearSlot)
            {
                return UGearBaseFunctionLibrary::BuildModifierPoolFromGearWithBase(
                    Item, Base, Definitions.AffixDefinitions);
            }
        }
    }

    return UGearAffixFunctionLibrary::BuildModifierPoolFromGear(
        Item, Definitions.AffixDefinitions);
}

bool UCharacterEquipmentAdapter::RefreshEquipmentStats(
    UCharacterStatsComponent* StatsComponent,
    const FCharacterEquipment& Equipment,
    const FEquipmentStatDefinitionSet& Definitions,
    EResourceRecalculationPolicy ResourcePolicy)
{
    if (!StatsComponent || !StatsComponent->IsStatMutationAuthority())
    {
        return false;
    }

    TArray<FCharacterStatSource> Sources;
    if (!BuildStatSources(Equipment, Definitions, Sources)) return false;

    return StatsComponent->ReplaceStatSourcesByType(
        ECharacterStatSourceType::Equipment, Sources, ResourcePolicy);
}

bool UCharacterEquipmentAdapter::BuildStatSources(
    const FCharacterEquipment& Equipment,
    const FEquipmentStatDefinitionSet& Definitions,
    TArray<FCharacterStatSource>& OutSources)
{
    OutSources.Reset();
    OutSources.Reserve(Equipment.EquippedItems.Num());
    TSet<FName> SourceIds;

    for (const TPair<EEquipmentSlot, FGearItem>& Pair : Equipment.EquippedItems)
    {
        const FGearItem& Item = Pair.Value;
        if (Item.ItemId.IsNone() || SourceIds.Contains(Item.ItemId))
        {
            OutSources.Reset();
            return false;
        }

        FCharacterStatSource Source;
        Source.SourceId = Item.ItemId;
        EquipmentAdapterPrivate::ApplyContribution(
            Source, BuildContribution(Pair.Key, Item, Definitions));
        OutSources.Add(MoveTemp(Source));
        SourceIds.Add(Item.ItemId);
    }
    return true;
}

FEquipmentAdapterOutcome UCharacterEquipmentAdapter::CommitTransaction(
    UCharacterStatsComponent* StatsComponent,
    FCharacterEquipment& Equipment,
    TArray<FGearItem>& InventoryItems,
    int32 InventoryCapacity,
    const FCharacterEquipment& PreparedEquipment,
    int32 InventoryItemIndexToRemove,
    const TArray<FGearItem>& ItemsToReturn,
    const FEquipmentStatDefinitionSet& Definitions)
{
    FEquipmentAdapterOutcome Outcome;
    Outcome.DisplacedItems = ItemsToReturn;

    TArray<FGearItem> PreparedInventory = InventoryItems;
    if (InventoryItemIndexToRemove != INDEX_NONE)
    {
        if (!PreparedInventory.IsValidIndex(InventoryItemIndexToRemove))
        {
            Outcome.Result = EEquipmentAdapterResult::Failed_ItemNotFound;
            return Outcome;
        }
        PreparedInventory.RemoveAt(InventoryItemIndexToRemove);
    }
    PreparedInventory.Append(ItemsToReturn);

    if (InventoryCapacity > 0 && PreparedInventory.Num() > InventoryCapacity)
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_InventoryFull;
        return Outcome;
    }
    if (!HasValidUniqueItemIds(PreparedInventory, PreparedEquipment))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_DuplicateItemId;
        return Outcome;
    }
    TArray<FCharacterStatSource> PreparedSources;
    if (!BuildStatSources(PreparedEquipment, Definitions, PreparedSources))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_StatRefresh;
        return Outcome;
    }

    // Replacing or removing one item can lower attributes supplied by that
    // item. Validate the entire prepared loadout against its prospective
    // attributes so another equipped item cannot silently become invalid.
    FBaseAttributes ProspectiveAttributes = StatsComponent->BaseAttributes;
    for (const TPair<FName, FCharacterStatSource>& Pair : StatsComponent->StatSources)
    {
        if (!Pair.Value.bEnabled || Pair.Value.SourceType == ECharacterStatSourceType::Equipment)
        {
            continue;
        }
        ProspectiveAttributes.Strength += Pair.Value.BonusStrength;
        ProspectiveAttributes.Dexterity += Pair.Value.BonusDexterity;
        ProspectiveAttributes.Intelligence += Pair.Value.BonusIntelligence;
    }
    for (const FCharacterStatSource& Source : PreparedSources)
    {
        if (!Source.bEnabled) continue;
        ProspectiveAttributes.Strength += Source.BonusStrength;
        ProspectiveAttributes.Dexterity += Source.BonusDexterity;
        ProspectiveAttributes.Intelligence += Source.BonusIntelligence;
    }
    for (const TPair<EEquipmentSlot, FGearItem>& Pair : PreparedEquipment.EquippedItems)
    {
        const EEquipResult RequirementFailure =
            UCharacterEquipmentFunctionLibrary::GetRequirementFailure(
                Pair.Value,
                StatsComponent->Progression.CurrentLevel,
                ProspectiveAttributes.Strength,
                ProspectiveAttributes.Dexterity,
                ProspectiveAttributes.Intelligence);
        if (RequirementFailure != EEquipResult::Success)
        {
            Outcome.Result = EEquipmentAdapterResult::Failed_EquipRules;
            Outcome.EquipResult = RequirementFailure;
            return Outcome;
        }
    }

    for (const FCharacterStatSource& Source : PreparedSources)
    {
        if (const FCharacterStatSource* Existing = StatsComponent->StatSources.Find(Source.SourceId))
        {
            if (Existing->SourceType != ECharacterStatSourceType::Equipment)
            {
                Outcome.Result = EEquipmentAdapterResult::Failed_StatRefresh;
                return Outcome;
            }
        }
    }

    const FCharacterEquipment OriginalEquipment = Equipment;
    const TArray<FGearItem> OriginalInventory = InventoryItems;
    Equipment = PreparedEquipment;
    InventoryItems = MoveTemp(PreparedInventory);

    if (!StatsComponent->ReplaceStatSourcesByType(
            ECharacterStatSourceType::Equipment,
            PreparedSources,
            EResourceRecalculationPolicy::PreserveCurrent))
    {
        Equipment = OriginalEquipment;
        InventoryItems = OriginalInventory;
        Outcome.Result = EEquipmentAdapterResult::Failed_StatRefresh;
        return Outcome;
    }

    Outcome.Result = EEquipmentAdapterResult::Success;
    Outcome.EquipResult = EEquipResult::Success;
    return Outcome;
}

FEquipmentAdapterOutcome UCharacterEquipmentAdapter::EquipItemFromInventory(
    UCharacterStatsComponent* StatsComponent,
    FCharacterEquipment& Equipment,
    TArray<FGearItem>& InventoryItems,
    int32 InventoryCapacity,
    EEquipmentSlot Slot,
    FName ItemId,
    const FEquipmentStatDefinitionSet& Definitions)
{
    FEquipmentAdapterOutcome Outcome;
    if (!StatsComponent)
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_StatsComponentUnavailable;
        return Outcome;
    }
    if (!StatsComponent->IsStatMutationAuthority())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_NotAuthority;
        return Outcome;
    }
    if (ItemId.IsNone())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_InvalidItemId;
        return Outcome;
    }
    if (IsItemAlreadyEquipped(Equipment, ItemId))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_ItemAlreadyEquipped;
        return Outcome;
    }

    const int32 ItemIndex = FindInventoryItemIndex(InventoryItems, ItemId);
    if (!InventoryItems.IsValidIndex(ItemIndex))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_ItemNotFound;
        return Outcome;
    }

    FCharacterEquipment PreparedEquipment = Equipment;
    const FBaseAttributes& Attributes = StatsComponent->CalculatedAttributes;
    const FEquipOutcome EquipOutcome = UCharacterEquipmentFunctionLibrary::EquipItem(
        PreparedEquipment, Slot, InventoryItems[ItemIndex],
        StatsComponent->Progression.CurrentLevel,
        Attributes.Strength, Attributes.Dexterity, Attributes.Intelligence);
    Outcome.EquipResult = EquipOutcome.Result;
    if (!EquipOutcome.Succeeded())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_EquipRules;
        return Outcome;
    }

    TArray<FGearItem> ItemsToReturn;
    EquipmentAdapterPrivate::AddDisplacedItem(
        ItemsToReturn, EquipOutcome.bHadPreviousItem, EquipOutcome.PreviousItem);
    return CommitTransaction(StatsComponent, Equipment, InventoryItems,
                             InventoryCapacity, PreparedEquipment, ItemIndex,
                             ItemsToReturn, Definitions);
}

FEquipmentAdapterOutcome UCharacterEquipmentAdapter::EquipMainHandFromInventory(
    UCharacterStatsComponent* StatsComponent,
    FCharacterEquipment& Equipment,
    TArray<FGearItem>& InventoryItems,
    int32 InventoryCapacity,
    FName ItemId,
    EWeaponCategory WeaponCategory,
    const FEquipmentStatDefinitionSet& Definitions)
{
    FEquipmentAdapterOutcome Outcome;
    if (!StatsComponent)
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_StatsComponentUnavailable;
        return Outcome;
    }
    if (!StatsComponent->IsStatMutationAuthority())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_NotAuthority;
        return Outcome;
    }
    if (ItemId.IsNone())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_InvalidItemId;
        return Outcome;
    }
    if (IsItemAlreadyEquipped(Equipment, ItemId))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_ItemAlreadyEquipped;
        return Outcome;
    }

    const int32 ItemIndex = FindInventoryItemIndex(InventoryItems, ItemId);
    if (!InventoryItems.IsValidIndex(ItemIndex))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_ItemNotFound;
        return Outcome;
    }

    FCharacterEquipment PreparedEquipment = Equipment;
    const FBaseAttributes& Attributes = StatsComponent->CalculatedAttributes;
    const FMainHandEquipOutcome EquipOutcome =
        UCharacterEquipmentFunctionLibrary::EquipMainHandWeapon(
            PreparedEquipment, InventoryItems[ItemIndex], WeaponCategory,
            StatsComponent->Progression.CurrentLevel,
            Attributes.Strength, Attributes.Dexterity, Attributes.Intelligence);
    Outcome.EquipResult = EquipOutcome.Result;
    if (!EquipOutcome.Succeeded())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_EquipRules;
        return Outcome;
    }

    TArray<FGearItem> ItemsToReturn;
    EquipmentAdapterPrivate::AddDisplacedItem(
        ItemsToReturn, EquipOutcome.bHadPreviousMainHand, EquipOutcome.PreviousMainHand);
    EquipmentAdapterPrivate::AddDisplacedItem(
        ItemsToReturn, EquipOutcome.bHadBumpedOffHand, EquipOutcome.BumpedOffHand);
    return CommitTransaction(StatsComponent, Equipment, InventoryItems,
                             InventoryCapacity, PreparedEquipment, ItemIndex,
                             ItemsToReturn, Definitions);
}

FEquipmentAdapterOutcome UCharacterEquipmentAdapter::EquipOffHandFromInventory(
    UCharacterStatsComponent* StatsComponent,
    FCharacterEquipment& Equipment,
    TArray<FGearItem>& InventoryItems,
    int32 InventoryCapacity,
    FName ItemId,
    EOffHandItemKind ItemKind,
    EWeaponCategory OffHandWeaponCategory,
    const FEquipmentStatDefinitionSet& Definitions)
{
    FEquipmentAdapterOutcome Outcome;
    if (!StatsComponent)
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_StatsComponentUnavailable;
        return Outcome;
    }
    if (!StatsComponent->IsStatMutationAuthority())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_NotAuthority;
        return Outcome;
    }
    if (ItemId.IsNone())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_InvalidItemId;
        return Outcome;
    }
    if (IsItemAlreadyEquipped(Equipment, ItemId))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_ItemAlreadyEquipped;
        return Outcome;
    }

    const int32 ItemIndex = FindInventoryItemIndex(InventoryItems, ItemId);
    if (!InventoryItems.IsValidIndex(ItemIndex))
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_ItemNotFound;
        return Outcome;
    }

    FCharacterEquipment PreparedEquipment = Equipment;
    const FBaseAttributes& Attributes = StatsComponent->CalculatedAttributes;
    const FEquipOutcome EquipOutcome = UCharacterEquipmentFunctionLibrary::EquipOffHand(
        PreparedEquipment, InventoryItems[ItemIndex], ItemKind, OffHandWeaponCategory,
        StatsComponent->Progression.CurrentLevel,
        Attributes.Strength, Attributes.Dexterity, Attributes.Intelligence);
    Outcome.EquipResult = EquipOutcome.Result;
    if (!EquipOutcome.Succeeded())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_EquipRules;
        return Outcome;
    }

    TArray<FGearItem> ItemsToReturn;
    EquipmentAdapterPrivate::AddDisplacedItem(
        ItemsToReturn, EquipOutcome.bHadPreviousItem, EquipOutcome.PreviousItem);
    return CommitTransaction(StatsComponent, Equipment, InventoryItems,
                             InventoryCapacity, PreparedEquipment, ItemIndex,
                             ItemsToReturn, Definitions);
}

FEquipmentAdapterOutcome UCharacterEquipmentAdapter::UnequipToInventory(
    UCharacterStatsComponent* StatsComponent,
    FCharacterEquipment& Equipment,
    TArray<FGearItem>& InventoryItems,
    int32 InventoryCapacity,
    EEquipmentSlot Slot,
    const FEquipmentStatDefinitionSet& Definitions)
{
    FEquipmentAdapterOutcome Outcome;
    if (!StatsComponent)
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_StatsComponentUnavailable;
        return Outcome;
    }
    if (!StatsComponent->IsStatMutationAuthority())
    {
        Outcome.Result = EEquipmentAdapterResult::Failed_NotAuthority;
        return Outcome;
    }

    FCharacterEquipment PreparedEquipment = Equipment;
    TArray<FGearItem> ItemsToReturn;

    if (Slot == EEquipmentSlot::MainHand)
    {
        const FMainHandEquipOutcome UnequipOutcome =
            UCharacterEquipmentFunctionLibrary::UnequipMainHandWeapon(PreparedEquipment);
        Outcome.EquipResult = UnequipOutcome.Result;
        if (!UnequipOutcome.Succeeded())
        {
            Outcome.Result = EEquipmentAdapterResult::Failed_EquipRules;
            return Outcome;
        }
        EquipmentAdapterPrivate::AddDisplacedItem(
            ItemsToReturn, UnequipOutcome.bHadPreviousMainHand, UnequipOutcome.PreviousMainHand);
        EquipmentAdapterPrivate::AddDisplacedItem(
            ItemsToReturn, UnequipOutcome.bHadBumpedOffHand, UnequipOutcome.BumpedOffHand);
    }
    else
    {
        const FEquipOutcome UnequipOutcome =
            UCharacterEquipmentFunctionLibrary::UnequipSlot(PreparedEquipment, Slot);
        Outcome.EquipResult = UnequipOutcome.Result;
        if (!UnequipOutcome.Succeeded())
        {
            Outcome.Result = EEquipmentAdapterResult::Failed_EquipRules;
            return Outcome;
        }
        EquipmentAdapterPrivate::AddDisplacedItem(
            ItemsToReturn, UnequipOutcome.bHadPreviousItem, UnequipOutcome.PreviousItem);
    }

    return CommitTransaction(StatsComponent, Equipment, InventoryItems,
                             InventoryCapacity, PreparedEquipment, INDEX_NONE,
                             ItemsToReturn, Definitions);
}
