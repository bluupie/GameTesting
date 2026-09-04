#include "CharacterEquipmentFunctionLibrary.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

EGearSlot UCharacterEquipmentFunctionLibrary::GetExpectedGearSlot(EEquipmentSlot Slot)
{
    switch (Slot)
    {
        case EEquipmentSlot::Helmet: return EGearSlot::Helmet;
        case EEquipmentSlot::Chest:  return EGearSlot::Chest;
        case EEquipmentSlot::Gloves: return EGearSlot::Gloves;
        case EEquipmentSlot::Boots:  return EGearSlot::Boots;
        case EEquipmentSlot::Ring1:
        case EEquipmentSlot::Ring2:  return EGearSlot::Ring;
        case EEquipmentSlot::Amulet: return EGearSlot::Amulet;
        case EEquipmentSlot::Belt:   return EGearSlot::Belt;
        default:                     return EGearSlot::Helmet; // MainHand/OffHand never reach here — see EquipItem
    }
}

// ---------------------------------------------------------------------------
// Simple slots
// ---------------------------------------------------------------------------

FEquipOutcome UCharacterEquipmentFunctionLibrary::EquipItem(FCharacterEquipment& Equipment, EEquipmentSlot Slot, const FGearItem& Item)
{
    FEquipOutcome Outcome;

    if (Slot == EEquipmentSlot::MainHand || Slot == EEquipmentSlot::OffHand)
    {
        Outcome.Result = EEquipResult::Failed_UseWeaponEquipFunction;
        return Outcome;
    }

    if (Item.GearSlot != GetExpectedGearSlot(Slot))
    {
        Outcome.Result = EEquipResult::Failed_WrongItemSlot;
        return Outcome;
    }

    if (const FGearItem* Previous = Equipment.GetEquippedItem(Slot))
    {
        Outcome.bHadPreviousItem = true;
        Outcome.PreviousItem = *Previous;
    }

    Equipment.EquippedItems.Add(Slot, Item);
    Outcome.Result = EEquipResult::Success;
    return Outcome;
}

FEquipOutcome UCharacterEquipmentFunctionLibrary::UnequipSlot(FCharacterEquipment& Equipment, EEquipmentSlot Slot)
{
    FEquipOutcome Outcome;

    if (Slot == EEquipmentSlot::MainHand)
    {
        Outcome.Result = EEquipResult::Failed_UseWeaponEquipFunction;
        return Outcome;
    }

    const FGearItem* Existing = Equipment.GetEquippedItem(Slot);
    if (!Existing)
    {
        Outcome.Result = EEquipResult::Failed_SlotAlreadyEmpty;
        return Outcome;
    }

    Outcome.bHadPreviousItem = true;
    Outcome.PreviousItem = *Existing;
    Equipment.EquippedItems.Remove(Slot);
    Outcome.Result = EEquipResult::Success;
    return Outcome;
}

// ---------------------------------------------------------------------------
// MainHand
// ---------------------------------------------------------------------------

FMainHandEquipOutcome UCharacterEquipmentFunctionLibrary::EquipMainHandWeapon(FCharacterEquipment& Equipment, const FGearItem& WeaponItem,
                                                                               EWeaponCategory WeaponCategory)
{
    FMainHandEquipOutcome Outcome;

    if (WeaponItem.GearSlot != EGearSlot::Weapon)
    {
        Outcome.Result = EEquipResult::Failed_WrongItemSlot;
        return Outcome;
    }

    if (const FGearItem* PreviousMainHand = Equipment.GetEquippedItem(EEquipmentSlot::MainHand))
    {
        Outcome.bHadPreviousMainHand = true;
        Outcome.PreviousMainHand = *PreviousMainHand;
    }

    if (IsTwoHandedWeapon(WeaponCategory))
    {
        // The new weapon claims OffHand outright — bump whatever was there.
        if (const FGearItem* PreviousOffHand = Equipment.GetEquippedItem(EEquipmentSlot::OffHand))
        {
            Outcome.bHadBumpedOffHand = true;
            Outcome.BumpedOffHand = *PreviousOffHand;
            Equipment.EquippedItems.Remove(EEquipmentSlot::OffHand);
        }
    }
    else if (const FGearItem* CurrentOffHand = Equipment.GetEquippedItem(EEquipmentSlot::OffHand))
    {
        // One-handed weapon — OffHand stays, but only if it's still valid
        // for what the NEW weapon allows: a Quiver becomes invalid when
        // switching from a Bow to a Sword, and a dual-wielded weapon
        // becomes invalid if its type no longer matches the new MainHand
        // weapon (e.g. a dual-wielded Axe is invalid once MainHand becomes
        // a Sword — dual wield requires matching types).
        const EOffHandCompatibility NewCompat = GetOffHandCompatibility(WeaponCategory);
        bool bStillValid =
            (NewCompat == EOffHandCompatibility::ShieldOrDualWield && Equipment.CurrentOffHandKind != EOffHandItemKind::Quiver) ||
            (NewCompat == EOffHandCompatibility::QuiverOnly && Equipment.CurrentOffHandKind == EOffHandItemKind::Quiver);

        if (bStillValid && Equipment.CurrentOffHandKind == EOffHandItemKind::DualWieldWeapon
            && Equipment.CurrentOffHandWeaponCategory != WeaponCategory)
        {
            bStillValid = false;
        }

        if (!bStillValid)
        {
            Outcome.bHadBumpedOffHand = true;
            Outcome.BumpedOffHand = *CurrentOffHand;
            Equipment.EquippedItems.Remove(EEquipmentSlot::OffHand);
        }
    }

    Equipment.EquippedItems.Add(EEquipmentSlot::MainHand, WeaponItem);
    Equipment.MainHandWeaponCategory = WeaponCategory;
    Outcome.Result = EEquipResult::Success;
    return Outcome;
}

FMainHandEquipOutcome UCharacterEquipmentFunctionLibrary::UnequipMainHandWeapon(FCharacterEquipment& Equipment)
{
    FMainHandEquipOutcome Outcome;

    const FGearItem* ExistingMainHand = Equipment.GetEquippedItem(EEquipmentSlot::MainHand);
    if (!ExistingMainHand)
    {
        Outcome.Result = EEquipResult::Failed_SlotAlreadyEmpty;
        return Outcome;
    }

    Outcome.bHadPreviousMainHand = true;
    Outcome.PreviousMainHand = *ExistingMainHand;
    Equipment.EquippedItems.Remove(EEquipmentSlot::MainHand);

    // With no MainHand weapon, only a bare Shield remains a valid OffHand
    // occupant — a Quiver or dual-wielded weapon has nothing to pair with.
    if (const FGearItem* CurrentOffHand = Equipment.GetEquippedItem(EEquipmentSlot::OffHand))
    {
        if (Equipment.CurrentOffHandKind != EOffHandItemKind::Shield)
        {
            Outcome.bHadBumpedOffHand = true;
            Outcome.BumpedOffHand = *CurrentOffHand;
            Equipment.EquippedItems.Remove(EEquipmentSlot::OffHand);
        }
    }

    Outcome.Result = EEquipResult::Success;
    return Outcome;
}

// ---------------------------------------------------------------------------
// OffHand
// ---------------------------------------------------------------------------

FEquipOutcome UCharacterEquipmentFunctionLibrary::EquipOffHand(FCharacterEquipment& Equipment, const FGearItem& OffHandItem,
                                                                 EOffHandItemKind ItemKind, EWeaponCategory OffHandWeaponCategory)
{
    FEquipOutcome Outcome;

    // A Shield carries GearSlot::Shield, a Quiver carries GearSlot::Quiver,
    // and a dual-wielded weapon carries GearSlot::Weapon regardless of
    // which hand it ends up in. This now also catches a Shield passed in
    // with ItemKind::Quiver (or vice versa), which the old single OffHand
    // value couldn't distinguish.
    EGearSlot ExpectedSlot = EGearSlot::Weapon;
    switch (ItemKind)
    {
        case EOffHandItemKind::Shield:          ExpectedSlot = EGearSlot::Shield; break;
        case EOffHandItemKind::Quiver:           ExpectedSlot = EGearSlot::Quiver; break;
        case EOffHandItemKind::DualWieldWeapon:  ExpectedSlot = EGearSlot::Weapon; break;
    }
    if (OffHandItem.GearSlot != ExpectedSlot)
    {
        Outcome.Result = EEquipResult::Failed_WrongItemSlot;
        return Outcome;
    }

    if (Equipment.IsSlotOccupied(EEquipmentSlot::MainHand))
    {
        const EOffHandCompatibility Compat = GetOffHandCompatibility(Equipment.MainHandWeaponCategory);

        if (Compat == EOffHandCompatibility::BlockedByTwoHanded)
        {
            Outcome.Result = EEquipResult::Failed_OffHandBlockedByTwoHanded;
            return Outcome;
        }
        if (Compat == EOffHandCompatibility::QuiverOnly && ItemKind != EOffHandItemKind::Quiver)
        {
            Outcome.Result = EEquipResult::Failed_IncompatibleOffHand; // Bow only pairs with a Quiver
            return Outcome;
        }
        if (Compat == EOffHandCompatibility::ShieldOrDualWield && ItemKind == EOffHandItemKind::Quiver)
        {
            Outcome.Result = EEquipResult::Failed_IncompatibleOffHand; // a Quiver only pairs with a Bow
            return Outcome;
        }
    }
    else if (ItemKind != EOffHandItemKind::Shield)
    {
        // No MainHand weapon: a Quiver has nothing to shoot from, and
        // dual-wielding needs a MainHand weapon to pair with. Only a bare
        // Shield is valid with an empty MainHand.
        Outcome.Result = EEquipResult::Failed_IncompatibleOffHand;
        return Outcome;
    }

    if (ItemKind == EOffHandItemKind::DualWieldWeapon)
    {
        if (IsTwoHandedWeapon(OffHandWeaponCategory))
        {
            Outcome.Result = EEquipResult::Failed_DualWieldRequiresOneHanded;
            return Outcome;
        }
        if (Equipment.IsSlotOccupied(EEquipmentSlot::MainHand) && OffHandWeaponCategory != Equipment.MainHandWeaponCategory)
        {
            // Dual wield requires matching weapon types — Sword+Sword,
            // Axe+Axe, Wand+Wand, Sceptre+Sceptre. A Sword+Axe combo, for
            // instance, is not allowed.
            Outcome.Result = EEquipResult::Failed_DualWieldRequiresMatchingType;
            return Outcome;
        }
    }

    if (const FGearItem* Previous = Equipment.GetEquippedItem(EEquipmentSlot::OffHand))
    {
        Outcome.bHadPreviousItem = true;
        Outcome.PreviousItem = *Previous;
    }

    Equipment.EquippedItems.Add(EEquipmentSlot::OffHand, OffHandItem);
    Equipment.CurrentOffHandKind = ItemKind;
    Equipment.CurrentOffHandWeaponCategory = OffHandWeaponCategory;
    Outcome.Result = EEquipResult::Success;
    return Outcome;
}