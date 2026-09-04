#pragma once

#include "CoreMinimal.h"
#include "CharacterStats.generated.h"

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
    int32 Strength = 10; // Suffix -- All

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Dexterity = 10; // Suffix - All

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Intelligence = 10; // Suffix -- All
};

// ---------------------------------------------------------------------------
// Stat Types
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EStatType : uint8
{
    Life, // Prefix -- Everything but Weapons
    Mana, // Prefix -- Everything
    Barrier, // Prefix -- Everything But Weaopons
    DamagePhysical, // Prefix -- Weapons, Gloves, Rings, Weapons
    DamageFire, // Prefix -- Weapons, Gloves, Rings, Weapons
    DamageCold, // Prefix -- Weapons, Gloves, Ring, sWeapons
    DamageLightning, // Prefix -- Weapons, Gloves, Rings, Weapons 
    DamagePoison, // Prefix -- Weapons, Gloves, Rings
    CriticalStrikeChance, // Suffix -- Weapons, Rings
    CriticalStrikeMultiplier, // Suffix -- Weapons, Rings
    AttackSpeed, // Suffix -- Weapons (Phsycial), Rings, Gloves
    MovementSpeed, // Prefix -- Boots
    Armour, // Prefix -- Everything but weapons
    FireResistance, // Prefix -- Everything but weapons
    ColdResistance, // Prefix -- Everything but weapons
    LightningResistance, // Prefix -- Everything but weapons
    PoisonResistance, // Prefix -- Everything but weapons
    Evasion, // Prefix -- Everything but weapons
    BlockChance, // Prefix -- Shields
    LifeLeech, // Suffix -- Gloves, Weapons, Rings
    ManaLeech, // Suffix -- Gloves, Weapons, Rings
    // Accuracy,
    DodgeChance, // Not an affix, but a stat solely inncreased by evasion
    SpellBlock, // Prefix -- Shields
    SpellDamage, // Prefix -- Weapons (Caster)
    CastSpeed, // Suffix -- Weapons (Caster), Rings, Gloves
    CooldownReduction, // Undetermined
    HealthRegen, // Suffix -- Everything but weapons
    ManaRegen, // Suffix -- Everything but weapons
    AoeRadius // Undetermined
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

    void Recalculate(const FBaseAttributes& Attrs, const FModifierPool& Pool, int32 Level)
    {
        auto Value = [&Pool](EStatType Type, float Base) -> float
        {
            const FStatModifier* Mod = Pool.Mods.Find(Type);
            if (!Mod)
            {
                return Base;
            }
            return (Base + Mod->Flat) * (1.0f + Mod->Percent) * Mod->MorePercent;
        };

        MaximumHealth = Value(EStatType::Life, 100.0f + Attrs.Strength * 5.0f + Level * 15.0f);
        MaximumMana   = Value(EStatType::Mana, 50.0f + Attrs.Intelligence * 5.0f + Level * 8.0f);
        EnergyShield  = Value(EStatType::EnergyShield, Attrs.Intelligence * 1.0f);

        // Regen was previously never recalculated here and stayed frozen at its
        // construction default regardless of Attrs/Level/modifiers.
        LifeRegeneration = Value(EStatType::HealthRegen, MaximumHealth * 0.01f);
        ManaRegeneration = Value(EStatType::ManaRegen, MaximumMana * 0.01f);

        DamagePhysical  = Value(EStatType::DamagePhysical, 10.0f + Attrs.Strength * 0.5f);
        DamageFire      = Value(EStatType::DamageFire, 0.0f);
        DamageCold      = Value(EStatType::DamageCold, 0.0f);
        DamageLightning = Value(EStatType::DamageLightning, 0.0f);
        DamagePoison    = Value(EStatType::DamagePoison, 0.0f);
        SpellDamage     = Value(EStatType::SpellDamage, 0.0f);

        // Both stored as fractions (0.05 = 5%, 1.5 = 150%) — see field comments above.
        CriticalStrikeChance     = Value(EStatType::CriticalStrikeChance, 0.05f + Attrs.Dexterity * 0.0005f);
        CriticalStrikeMultiplier = Value(EStatType::CriticalStrikeMultiplier, 1.5f);
        AttackSpeed = Value(EStatType::AttackSpeed, 1.0f);
        CastSpeed   = Value(EStatType::CastSpeed, 1.0f);
        Accuracy    = Value(EStatType::Accuracy, 100.0f + Attrs.Dexterity * 2.0f);
        LifeLeech   = Value(EStatType::LifeLeech, 0.0f);
        ManaLeech   = Value(EStatType::ManaLeech, 0.0f);

        Armour  = Value(EStatType::Armour, Attrs.Strength * 2.0f);
        Evasion = Value(EStatType::Evasion, Attrs.Dexterity * 2.0f);
        Barrier = Value(EStatType::Barrier, Attrs.Intelligence * 2.0f);

        BlockChance      = Value(EStatType::BlockChance, 0.0f);
        SpellBlockChance = Value(EStatType::SpellBlock, 0.0f);
        DodgeChance      = Value(EStatType::DodgeChance, 0.0f);

        FireResistance      = Value(EStatType::FireResistance, 0.0f);
        ColdResistance      = Value(EStatType::ColdResistance, 0.0f);
        LightningResistance = Value(EStatType::LightningResistance, 0.0f);
        PoisonResistance    = Value(EStatType::PoisonResistance, 0.0f);

        MovementSpeed     = Value(EStatType::MovementSpeed, 1.0f);
        AoeRadius         = Value(EStatType::AoeRadius, 1.0f);
        CooldownReduction = Value(EStatType::CooldownReduction, 0.0f);

        // CurrentHealth/CurrentMana intentionally NOT reset here — clamp instead,
        // so a stat recalc mid-fight doesn't fully heal/refill the player.
        CurrentHealth = FMath::Min(CurrentHealth, MaximumHealth);
        CurrentMana   = FMath::Min(CurrentMana, MaximumMana);
    }
};
