#include "CharacterStatsTypes.h"

void FCharacterStats::Recalculate(const FBaseAttributes& Attrs, const FModifierPool& Pool, int32 Level)
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
    MaximumMana = Value(EStatType::Mana, 50.0f + Attrs.Intelligence * 5.0f + Level * 8.0f);
    EnergyShield = Value(EStatType::EnergyShield, Attrs.Intelligence * 1.0f);

    LifeRegeneration = Value(EStatType::HealthRegen, MaximumHealth * 0.01f);
    ManaRegeneration = Value(EStatType::ManaRegen, MaximumMana * 0.01f);

    DamagePhysical = Value(EStatType::DamagePhysical, 10.0f + Attrs.Strength * 0.5f);
    DamageFire = Value(EStatType::DamageFire, 0.0f);
    DamageCold = Value(EStatType::DamageCold, 0.0f);
    DamageLightning = Value(EStatType::DamageLightning, 0.0f);
    DamagePoison = Value(EStatType::DamagePoison, 0.0f);
    SpellDamage = Value(EStatType::SpellDamage, 0.0f);

    // Fractions: 0.05 is 5% critical chance and 1.5 is 150% critical damage.
    CriticalStrikeChance = Value(EStatType::CriticalStrikeChance, 0.05f + Attrs.Dexterity * 0.0005f);
    CriticalStrikeMultiplier = Value(EStatType::CriticalStrikeMultiplier, 1.5f);
    AttackSpeed = Value(EStatType::AttackSpeed, 1.0f);
    CastSpeed = Value(EStatType::CastSpeed, 1.0f);
    Accuracy = Value(EStatType::Accuracy, 100.0f + Attrs.Dexterity * 2.0f);
    LifeLeech = Value(EStatType::LifeLeech, 0.0f);
    ManaLeech = Value(EStatType::ManaLeech, 0.0f);
    DamageWithSwords = Value(EStatType::DamageWithSwords, 1.0f);
    DamageWithAxes = Value(EStatType::DamageWithAxes, 1.0f);
    ProjectileDamage = Value(EStatType::ProjectileDamage, 1.0f);
    AdditionalProjectiles = FMath::Max(0, FMath::RoundToInt(Value(EStatType::AdditionalProjectiles, 0.0f)));

    Armour = Value(EStatType::Armour, Attrs.Strength * 2.0f);
    Evasion = Value(EStatType::Evasion, Attrs.Dexterity * 2.0f);
    Barrier = Value(EStatType::Barrier, Attrs.Intelligence * 2.0f);
    BlockChance = Value(EStatType::BlockChance, 0.0f);
    BlockAmount = Value(EStatType::BlockAmount, 0.0f);
    SpellBlockChance = Value(EStatType::SpellBlock, 0.0f);
    DodgeChance = Value(EStatType::DodgeChance, 0.0f);
    FireResistance = Value(EStatType::FireResistance, 0.0f);
    ColdResistance = Value(EStatType::ColdResistance, 0.0f);
    LightningResistance = Value(EStatType::LightningResistance, 0.0f);
    PoisonResistance = Value(EStatType::PoisonResistance, 0.0f);

    MovementSpeed = Value(EStatType::MovementSpeed, 1.0f);
    AoeRadius = Value(EStatType::AoeRadius, 1.0f);
    CooldownReduction = Value(EStatType::CooldownReduction, 0.0f);

    if (!bHasCalculatedStats)
    {
        CurrentHealth = MaximumHealth;
        CurrentMana = MaximumMana;
        bHasCalculatedStats = true;
    }
    else
    {
        CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaximumHealth);
        CurrentMana = FMath::Clamp(CurrentMana, 0.0f, MaximumMana);
    }
}
