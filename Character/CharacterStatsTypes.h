#pragma once

#include "CoreMinimal.h"
#include "CharacterStatsTypes.generated.h"

// ---------------------------------------------------------------------------
// Character
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCharacterProgression
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    FString ClassName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 CurrentLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 MaximumLevel = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 CurrentExperience = 0;

    // NOTE: UFUNCTION is not legal inside a USTRUCT (UnrealHeaderTool only
    // reflects functions on UCLASS/UINTERFACE types). This was previously
    // marked UFUNCTION(BlueprintPure, ...), which would fail UHT parsing.
    // If Blueprint access is needed, expose this via a UFUNCTION on a
    // UCLASS (e.g. a BlueprintFunctionLibrary) that takes/returns this struct.
    int32 GetExperienceForNextLevel() const
    {
        if (CurrentLevel >= MaximumLevel)
        {
            return 0; // Already at max level — nothing more needed.
        }
        return 1000 + (CurrentLevel - 1) * 500; // Example formula for experience needed
    }
};

// ---------------------------------------------------------------------------
// Base Attributes
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FBaseAttributes
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Strength = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Dexterity = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Intelligence = 10;
};

// ---------------------------------------------------------------------------
// Stat Types
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EStatType : uint8
{
    Life,
    Mana,
    Barrier,
    DamagePhysical,
    DamageFire,
    DamageCold,
    DamageLightning,
    DamagePoison,
    CriticalStrikeChance,
    CriticalStrikeMultiplier,
    AttackSpeed,
    MovementSpeed,
    Armour,
    FireResistance,
    ColdResistance,
    LightningResistance,
    PoisonResistance,
    Evasion,
    BlockChance,
    BlockAmount,
    LifeLeech,
    ManaLeech,
    EnergyShield,
    Accuracy,
    DodgeChance,
    SpellBlock,
    SpellDamage,
    CastSpeed,
    CooldownReduction,
    HealthRegen,
    ManaRegen,
    AoeRadius,
    DamageWithSwords,
    DamageWithAxes,
    ProjectileDamage,
    AdditionalProjectiles
};

// ---------------------------------------------------------------------------
// Stat Modifier
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FStatModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    float Flat = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    float Percent = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    float MorePercent = 1.0f;
};

// ---------------------------------------------------------------------------
// Modifier Pool
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FModifierPool
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
    TMap<EStatType, FStatModifier> Mods;

    void AddFlat(EStatType Type, float Value)
    {
        Mods.FindOrAdd(Type).Flat += Value;
    }

    void AddIncreased(EStatType Type, float Value)
    {
        Mods.FindOrAdd(Type).Percent += Value;
    }

    void AddMore(EStatType Type, float Value)
    {
        Mods.FindOrAdd(Type).MorePercent *= (1.0f + Value);
    }

    void Clear()
    {
        Mods.Empty();
    }
};

// ---------------------------------------------------------------------------
// Character Stats
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCharacterStats
{
    GENERATED_BODY()

    // Runtime guard used to distinguish the first stat calculation (spawn/load)
    // from later recalculations caused by equipment or temporary effects.
    UPROPERTY(Transient)
    bool bHasCalculatedStats = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float MaximumHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float CurrentHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float LifeRegeneration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float MaximumMana = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float CurrentMana = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float ManaRegeneration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float EnergyShield = 0.0f;

    // Damage Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float DamagePhysical = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float DamageFire = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float DamageCold = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float DamageLightning = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float DamagePoison = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float SpellDamage = 0.0f;

    // NOTE: stored as a fraction (0.05 = 5%), matching CriticalStrikeMultiplier's
    // convention below (1.5 = 150%). Format as a percentage only at display time —
    // do NOT flip this back to percent-scale without also updating Recalculate().
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float CriticalStrikeChance = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float CriticalStrikeMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float AttackSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float CastSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float Accuracy = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float LifeLeech = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float ManaLeech = 0.0f;

    // Conditional combat multipliers. The combat/ability layer applies the
    // matching value after identifying the equipped weapon or skill tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float DamageWithSwords = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float DamageWithAxes = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    float ProjectileDamage = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Damage")
    int32 AdditionalProjectiles = 0;

    // Defense Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float Armour = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float Evasion = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float Barrier = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float BlockChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float BlockAmount = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float SpellBlockChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float DodgeChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float FireResistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float ColdResistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float LightningResistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Defense")
    float PoisonResistance = 0.0f;

    // Utility Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Utility")
    float MovementSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Utility")
    float AoeRadius = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Utility")
    float CooldownReduction = 0.0f;

    void Recalculate(const FBaseAttributes& Attrs, const FModifierPool& Pool, int32 Level);
};
