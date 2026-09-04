#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Gear/GearAffixFunctionLibrary.h"
#include "Gear/GearBaseTypes.h"
#include "GearBaseFunctionLibrary.generated.h"

UCLASS()
class UGearBaseFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Linearly interpolates RequiredCharacterLevel and BaseItemLevel for
    // Rank (1..TotalRanks) between the given endpoints.
    //
    // IMPORTANT: Rank 1 is the EARLIEST/WEAKEST base and Rank TotalRanks is
    // the LATEST/STRONGEST — the OPPOSITE convention from
    // FAffixTier::TierNumber, where Tier 1 is the rare/best roll and Tier 9
    // is the common/worst one. "Base rank" and "affix tier" climb in
    // opposite directions; don't conflate them.
    UFUNCTION(BlueprintPure, Category = "Gear Bases")
    static FGearBaseLevelProgression GetBaseLevelProgression(int32 Rank, int32 TotalRanks,
                                                               int32 MinCharacterLevel = 1, int32 MaxCharacterLevel = 80,
                                                               int32 MinItemLevel = 1, int32 MaxItemLevel = 90);

    // Illustrative seed data: 9 ranks x 4 armour slots (Helmet/Chest/Gloves/
    // Boots) x 3 defense types (Armour/Evasion/Barrier) = 108 bases.
    // RequiredCharacterLevel spans 1-80 and BaseItemLevel spans 1-90 across
    // the 9 ranks per slot+defense combo (see GetBaseLevelProgression) — the
    // top rank's ilvl 90 comfortably clears the affix example table's Tier 1
    // requirement of 85, so best-in-slot gear can roll every affix tier.
    //
    // NOT BlueprintPure — see GetExampleAffixTable's comment for why: this
    // deep-copies a 108-entry table on every call.
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetExampleArmourBaseTable();

    // Illustrative seed data: 5 ranks x 3 defense types = 15 shield bases.
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetExampleShieldBaseTable();

    // Illustrative seed data: 5 quiver bases (no defense type).
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetExampleQuiverBaseTable();

    // Convenience: all three tables concatenated.
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FGearBaseItem> GetAllExampleGearBases();

    // Illustrative seed data for the defense-type affix restriction list —
    // see FDefenseTypeAffixRestriction's comment. This is the single place
    // that encodes "Armour gear can't roll Mana", etc. Add a row here (or,
    // once this is a UDataTable, a row in the sheet) to add a restriction;
    // delete one to remove it. No other code needs to change either way.
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FDefenseTypeAffixRestriction> GetExampleDefenseTypeRestrictions();

    // True if RestrictedAffixGroup contains a row barring AffixGroup from
    // rolling on DefenseType gear.
    UFUNCTION(BlueprintPure, Category = "Gear Bases")
    static bool IsAffixGroupRestrictedForDefenseType(const TArray<FDefenseTypeAffixRestriction>& Restrictions,
                                                       FName AffixGroup, EDefenseType DefenseType);

    // Returns AffixPool with every affix whose AffixGroup is restricted for
    // DefenseType removed. Called internally by GenerateGearItemFromBase on
    // every roll — fine for occasional/moderate item generation. If
    // generating items in bulk (e.g. a big loot table tick), call this once
    // per EDefenseType up front, cache the three results yourself, and pass
    // the matching pre-filtered array into GenerateGearItemFromBase's
    // AffixPool parameter instead of re-filtering the full pool every time.
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static TArray<FAffixDefinition> FilterAffixPoolForDefenseType(const TArray<FAffixDefinition>& AffixPool,
                                                                    const TArray<FDefenseTypeAffixRestriction>& Restrictions,
                                                                    EDefenseType DefenseType);

    // Generates a full item from a base: rolls affixes at Base.BaseItemLevel
    // via UGearAffixFunctionLibrary::GenerateGearItem, and stamps the
    // result's BaseItemId so BuildModifierPoolFromGearWithBase can look the
    // base back up later. When Base.bHasDefenseType is true, AffixPool is
    // first passed through FilterAffixPoolForDefenseType against Restrictions
    // so restricted affix groups can never roll on this item. Quivers
    // (bHasDefenseType == false) roll from AffixPool unfiltered. Pass an
    // empty Restrictions array to disable restriction filtering entirely.
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static FGearItem GenerateGearItemFromBase(const FGearBaseItem& Base, const TArray<FAffixDefinition>& AffixPool,
                                               const TArray<FDefenseTypeAffixRestriction>& Restrictions,
                                               EItemRarity Rarity, float RareFullAffixChance = 0.2f);

    // Combines a base's innate defense/block chance with its rolled affixes
    // into one contribution, ready for FBaseAttributes + FCharacterStats::Recalculate.
    UFUNCTION(BlueprintCallable, Category = "Gear Bases")
    static FGearStatContribution BuildModifierPoolFromGearWithBase(const FGearItem& Item, const FGearBaseItem& Base,
                                                                     const TArray<FAffixDefinition>& AffixPool);

private:
    static FString BuildArmourBaseName(EDefenseType DefenseType, EGearSlot Slot, int32 Rank);
    static FString BuildShieldBaseName(EDefenseType DefenseType, int32 Rank);
    static float LerpRankValue(int32 Rank, int32 TotalRanks, float MinValue, float MaxValue);
};