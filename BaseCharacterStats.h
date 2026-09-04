#pragma once
#include <"CoreMinimal.h">
#include <"CharacterStats.generated.h">   

#include <iostream>
#include <string>
#include <unordered_map>

USTRUCT(BlueprintType)
struct FCharacterProgression
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    FString ClassName

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 CurrentLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 MaximumLevel = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 CurrentExperience = 0;

    UFUNCTION(BlueprintPure, Category = "Character")
    int32 GetExperienceForNextLevel() const
    {
        return 1000 + (CurrentLevel - 1) * 500; // Example formula for experience needed
    }
};

USTRUCT(BlueprintType)
stuct FBaseAttributes
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Strength = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Dexterity = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Intelligence = 10;
};

UENUM(BlueprintType)
enum class EStatType : uint8
{
    Life, Mana, Barrier, DamagePhysical, DamageFire, DamageCold, DamageLightning, DamagePoison, CriticalStrikeChance, CriticalStrikeMultiplier, AttackSpeed, MovementSpeed, Armour, FireResistance, ColdResistance, LightningResistance, PoisonResistance, Armour, Evasion, BlockChance, BlockAmount, LifeLeech, ManaLeech, EnergyShield, Accuracy, DodgeChance, SpellBlock, SpellDamage, MovementSpeed, CastSpeed, CooldownReduction, HealthRegen, ManaRegen, AoeRadius
};

USTRUCT(BlueprintType)
struct FStatModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    float flat = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    float percent = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    float morePercent = 1.0f;
};

USTRUCT (BlueprintType)
struct FModifierPool
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
    TMap<EStatType, FStatModifier> mods;

    void AddFlat(EStatType type, float value) {
        mods.FindOrAdd(type).flat += value;
    }

    void AddIncreased(EStatType type, float value) {
        mods.FindOrAdd(type).percent += value;
    }

    void AddMore(EStatType type, float value) {
        mods.FindOrAdd(type).morePercent *= (1.0f + value);
    }

    void Clear() {
        mods.Empty();
    }
};

USTRUCT(BlueprintType)
struct FCharacterStats {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float maximumHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float currentHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float lifeRegeneration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float maximumMana = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float currentMana = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float manaRegeneration = 1.0f;

    // Damage Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float damagePhysical = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float damageFire = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float damageCold = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float damageLightning = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float damagePoison = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float criticalStrikeChance = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float criticalStrikeMultiplier = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float attackSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float castSpeed = 1.0f;

    // Defense Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float armour = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float evasion = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float barrier = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float blockChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float spellBlockChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float fireResistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float coldResistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float lightningResistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float poisonResistance = 0.0f;

    // Utility Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float movementSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float aoeRadius = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Resources")
    float cooldownReduction = 0.0f;

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
        
        MaximumHealth = Value(EStatType::Life, 100.0f + Attrs.Strength * 5.0f + Level * 8.0f);
        MaximumMana   = Value(EStatType::Mana, 50.0f + Attrs.Intelligence * 5.0f + Level * 4.0f);
 
        DamagePhysical  = Value(EStatType::DamagePhysical, 10.0f + Attrs.Strength * 0.5f);
        DamageFire      = Value(EStatType::DamageFire, 0.0f);
        DamageCold      = Value(EStatType::DamageCold, 0.0f);
        DamageLightning = Value(EStatType::DamageLightning, 0.0f);
        DamagePoison    = Value(EStatType::DamagePoison, 0.0f);
 
        CriticalStrikeChance     = Value(EStatType::CriticalStrikeChance, 0.05f + Attrs.Dexterity * 0.0005f);
        CriticalStrikeMultiplier = Value(EStatType::CriticalStrikeMultiplier, 1.5f);
        AttackSpeed = Value(EStatType::AttackSpeed, 1.0f);
        CastSpeed   = Value(EStatType::CastSpeed, 1.0f);
 
        Armour  = Value(EStatType::Armour, Attrs.Strength * 2.0f);
        Evasion = Value(EStatType::Evasion, Attrs.Dexterity * 2.0f);
        Barrier = Value(EStatType::Barrier, Attrs.Intelligence * 2.0f);
 
        BlockChance      = Value(EStatType::BlockChance, 0.0f);
        SpellBlockChance = Value(EStatType::SpellBlock, 0.0f);
 
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
};