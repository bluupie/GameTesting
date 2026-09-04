#pragma once

#include "CoreMinimal.h"
#include "GearAffixTypes.generated.h"
#include "BaseCharacterStats.h"
#include "Engine/DataTable.h"

UENUM(BlueprintType)
enum class EGearAffixType : uint8
{
    Prefix,
    Suffix
};

UENUM(BlueprintType)
enum class EGearSlot : uint8
{
    Weapon,
    OffHand,
    Helmet,
    Chest,
    Gloves,
    Boots,
    Belt,
    Amulet,
    Ring    
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Common,
    Magic,
    Rare,
    Legendary
};

UENUM(BlueprintType)
enum class EModifierApplication : uint8
{
    Flat,
    Increased,
    More
};

USTRUCT(BlueprintType)
struct FAffixTier
{
    Generated_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 TierLevel = 9;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float minRoll = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float maxRoll = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 RequiredItemLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct FAffixDefinition : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName AffixID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FString DisplayFormat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EAffixType AffixType = EAffixType::Prefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EStatType StatType = EStatType::MaximumHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EModifierApplication ModifierApplication = EModifierApplication::Flat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName AffixGroup;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    TArray<FAffixTier> Tiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float Weight = 100.0f;

    bool isEligibleForSlot(EGearSlot Slot) const
    {
        return AllowedSlots.Contains(Slot);
    };

    TArray<int32>GetUnlockedTierIndices(int32 ItemLevel) const
    {
        TArray<int32> Result;
        for (int32 i = 0; i < Tiers.Num(); ++i)
        {
            if (Tiers[i].RequiredItemLevel <= ItemLevel)
            {
                Result.Add(i);
            }
        }
        return Result;
    };

    USTRUCT(BlueprintType)
    struct FRolledAffix
    {
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
        FName AffixID;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
        int32 TierNumber = 9;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
        float RolledValue = 0.0f;
    };

    // Gear Item

    USTRUCT(BlueprintType)
    struct GearItem
    {
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
        FName ItemID;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
        FString GearName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
        EGearSlot GearSlot = EGearSlot::Weapon;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
        EItemRarity Rarity = EItemRarity::Common;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
        int32 ItemLevel;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
        TArray<FRolledAffix> PrefixAffixes;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gear")
        TArray<FRolledAffix> SuffixAffixes;

        int32 GetTotalAffixCount() const
        {
            return PrefixAffixes.Num() + SuffixAffixes.Num();
        }
    };
};