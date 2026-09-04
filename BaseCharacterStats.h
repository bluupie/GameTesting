#include <iostream>
#include <string>
#include <unordered_map>

struct Character {
    std::string name;
    std::string className;
    int currentLevel = 1;
    int maximumLevel = 100;
    int currentExp = 0;

    int ExpRequiredForLevel(int level) const {
        return 1000 + (level * level * 50); // placeholder curve
    }


};

struct BaseAttributes {
    int strength = 10;
    int dexterity = 10;
    int intelligence = 10;
};

enum class StatType {
    Life, Mana, Barrier, DamagePhysical, DamageFire, DamageCold, DamageLightning, DamagePoison, CriticalStrikeChance, CriticalStrikeMultiplier, AttackSpeed, MovementSpeed, Armor, fireResistance, coldResistance, lightningResistance, coldResistance, poisonResistance, Armour, Evasion, BlockChance, BlockAmount, LifeLeech, ManaLeech, EnergyShield, Accuracy, DodgeChance, SpellBlock, SpellDaamage, MovementSpeed, CastSpeed, CooldownReduction, HealthRegen, ManaRegen, AoeRadius
};

struct StatModifier {
    float flat  = 0.0f;
    float percent = 0.0f;
    float morePercent = 0.0f;
};

struct ModifierPool {
    std::unordered_map<StatType, StatModifier> mods;
 
    void AddFlat(StatType type, float amount) { mods[type].flat += amount; }
    void AddIncreased(StatType type, float percent) { mods[type].percent += percent; }
    void AddMore(StatType type, float percent) { mods[type].morePercent *= (1.0f + percent); }
 
    void Clear() { mods.clear(); }
};

struct CharacterStats {
    float maximumHealth = 100.0f;
    float currentHealth = 100.0f;
    float lifeRegeneration = 1.0f;
    float maximumMana = 50.0f;
    float currentMana = 50.0f;
    float manaRegeneration = 1.0f;

    // Damage Stats
    float damagePhysical = 10.0f;
    float damageFire = 0.0f;
    float damageCold = 0.0f;
    float damageLightning = 0.0f;
    float damagePoison = 0.0f;
    float criticalStrikeChance = 5.0f;
    float criticalStrikeMultiplier = 150.0f;
    float attackSpeed = 1.0f;
    float castSpeed = 1.0f;

    // Defense Stats
    float armor = 0.0f;
    float evasion = 0.0f;
    float barrier = 0.0f;
    float blockChance = 0.0f;
    float spellBlockChance = 0.0f;
    float fireResistance = 0.0f;
    float coldResistance = 0.0f;
    float lightningResistance = 0.0f;
    float poisonResistance = 0.0f;

    // Utility Stats
    float movementSpeed = 1.0f;
    float aoeRadius = 1.0f;
    float cooldownReduction = 0.0f;

    void Recalculate(const BaseAttributes& attrs, const ModifierPool& pool, int level) {
        auto value = [&](StatType type, float base) {
            auto it = pool.mods.find(type);
            if (it == pool.mods.end()) return base;
            const StatModifier& m = it->second;
            return (base + m.flat) * (1.0f + m.percent) * m.morePercent;
        };
 
        maximumHealth = value(StatType::Life, 100.0f + attrs.strength * 5.0f + level * 8.0f);
        maximumMana   = value(StatType::Mana, 50.0f + attrs.intelligence * 5.0f + level * 4.0f);
 
        damagePhysical  = value(StatType::DamagePhysical, 10.0f + attrs.strength * 0.5f);
        damageFire      = value(StatType::DamageFire, 0.0f);
        damageCold      = value(StatType::DamageCold, 0.0f);
        damageLightning = value(StatType::DamageLightning, 0.0f);
        damagePoison    = value(StatType::DamagePoison, 0.0f);
 
        criticalStrikeChance    = value(StatType::CriticalStrikeChance, 0.05f + attrs.dexterity * 0.0005f);
        criticalStrikeMultiplier = value(StatType::CriticalStrikeMultiplier, 1.5f);
        attackSpeed = value(StatType::AttackSpeed, 1.0f);
        castSpeed   = value(StatType::CastSpeed, 1.0f);
 
        armor    = value(StatType::Armor, attrs.strength * 2.0f);
        evasion  = value(StatType::Evasion, attrs.dexterity * 2.0f);
        barrier  = value(StatType::Barrier, attrs.intelligence * 2.0f);
 
        blockChance      = value(StatType::BlockChance, 0.0f);
        spellBlockChance = value(StatType::SpellBlock, 0.0f);
 
        fireResistance      = value(StatType::fireResistance, 0.0f);
        coldResistance      = value(StatType::coldResistance, 0.0f);
        lightningResistance = value(StatType::lightningResistance, 0.0f);
        poisonResistance    = value(StatType::poisonResistance, 0.0f);
 
        movementSpeed      = value(StatType::MovementSpeed, 1.0f);
        aoeRadius          = value(StatType::AoeRadius, 1.0f);
        cooldownReduction  = value(StatType::CooldownReduction, 0.0f);
 
        // currentHealth/currentMana intentionally NOT reset here — clamp instead,
        // so a stat recalc mid-fight doesn't fully heal/refill the player.
        if (currentHealth > maximumHealth) currentHealth = maximumHealth;
        if (currentMana > maximumMana) currentMana = maximumMana;
    };
};