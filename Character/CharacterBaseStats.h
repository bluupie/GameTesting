#include <iostream>
#include <string>
#include <unordered_map>

enum class CharacterClass {
    Sorceror,
    Warrior,
    Ranger,
    Duelist,
    Templar,
    Witch
};

struct Attributes {
    int Strength;
    int Intelligence;
    int Dexterity;
};

struct CoreStats {
    int maximumLife;
    float baseLifeRegeneration;
    int maximumMana;
    float baseManaRegeneration;
    int baseEvasion;
    float baseChancetoEvade;
    int baseArmour;
    float basePhysicalDamageReduction;
    int baseEnergyShield;
    float baseChancetoSuppressSpellDamage;
    int baseAccuracy;
    float baseMovementSpeed;
    float attackCriticalStrikeChance;
    float criticalStrikMultiplier;
    float spellCriticalStrikeChance;
    float chanceToBlock;
    float chanceToBlockSpellDamage;
    int resistanceFire;
    int resistanceCold;
    int resistanceLightning;
    int resistancePoison;

};

class Character {
    private:
    std::string m_name;
    CharacterClass m_class;
    int m_level;
    int m_experience;
    int m_experienceRequired;
    Attributes m_baseAttributes;
    CoreStats m_coreStats;

    public:
    Character(std::string name, CharacterClass charClass) 
        : m_name(std::move(name)), m_class(charClass), m_level(1) {
        setStartingAttributes();
    };

    void setStartingAttributes() {
        switch(m_class) {
            case CharacterClass::Warrior: m_baseAttributes = {32, 14, 14}; break;
            case CharacterClass::Ranger:   m_baseAttributes = {14, 32, 14}; break;
            case CharacterClass::Witch:    m_baseAttributes = {14, 14, 32}; break;
            case CharacterClass::Duelist:  m_baseAttributes = {23, 23, 14}; break;
            case CharacterClass::Templar:  m_baseAttributes = {23, 14, 23}; break;
            case CharacterClass::Sorceror: m_baseAttributes = {12, 12, 36}; break;
        };
    };

    void levelUp(int levels = 1) {
        if(m_level = 100){
            m_level += levels;
            m_coreStats.maximumLife + 12;
            m_coreStats.maximumMana + 6;
        } else{
            
        };
    };

    void gainExperience(int experienceGained){
        if (experienceGained >= m_experienceRequired) {
            levelUp();
        } else {
            m_experience += m_experienceRequired;
        };
    };

    Attributes getAttributes() const {
        return m_baseAttributes;
    };

    CoreStats calculalteCoreStats() const {
        CoreStats stats;

        // Per-level scaling additions
        int levelLifeBonus = (m_level - 1) * 12;
        int levelManaBonus = (m_level - 1) * 6;
        int levelEvasionBonus = (m_level - 1) * 3;
        int levelAccuracyBonus = (m_level - 1) * 2;

        // Attribute scaling calculations:
        // Every 10 Strength = +5 Life
        // Every 10 Intelligence = +5 Mana
        int lifeFromStr = (m_baseAttributes.Strength / 10) * 5;
        int armourFromStr = (m_baseAttributes.Strength / 5) * 5;
        int manaFromInt = (m_baseAttributes.Intelligence / 10) * 5;
        int energyShieldFromInt = (m_baseAttributes.Intelligence / 5) * 5;
        int evasionFromDex = (m_baseAttributes.Dexterity / 10) * 5;

        // Static baseline + level bonus + attribute bonus
        // Finish adding baseline stats
        stats.maximumLife = 50 + levelLifeBonus + lifeFromStr;
        stats.maximumMana = 40 + levelManaBonus + manaFromInt;
        stats.baseEvasion = 15 + levelEvasionBonus;
        stats.baseAccuracy = levelAccuracyBonus; // Flat class accuracy baseline starts at level scaling
        stats.baseManaRegeneration = 1.8f;
        stats.attackCriticalStrikeChance = 100.0f;
        stats.criticalStrikMultiplier = 150.0f;
        stats.baseMovementSpeed = 15.0f;
        // stats.baseChancetoSuppressSpellDamage = 5 + energyShieldFromInt / 2;
        // This will be a test when able too compile and verify scaling data if possible.
        return stats;
    };

    void DisplayStats() const {
        CoreStats currentStats = calculalteCoreStats();
        std::cout << "--- " << m_name << " (Level " << m_level << ") ---\n"
                //Attribute Block
                << "STR: " << m_baseAttributes.Strength
                << " | DEX: " << m_baseAttributes.Dexterity 
                << " | INT: " << m_baseAttributes.Intelligence << "\n"
                // Defensive Stats
                << "Life: " << currentStats.maximumLife 
                << " | Mana: " << currentStats.maximumMana << "\n"
                << "Evasion: " << currentStats.baseEvasion 
                << " | Accuracy: " << currentStats.baseAccuracy << "\n"
                << "Mana Regen: " << currentStats.baseManaRegeneration << "%/s"
                << " | Crit Multiplier: " << currentStats.criticalStrikMultiplier << "%\n\n";
    };

    int main() {
        // Instantiate a Sorceror and a duelist stats for testing, can change character class to simulate other classes when testing.
        //Character hero1("Gorg", CharacterClass::Sorceror);
        //Character hero2("Silas", CharacterClass::Duelist);

        //std::cout << "Initial Level 1 Baseline:\n";
        //hero1.DisplayStats();
        //hero2.DisplayStats();

        //std::cout << "Simulating progression to Level 20:\n";
        //hero1.levelUp(19);
        //hero2.levelUp(19);

        //hero1.DisplayStats();
        //hero2.DisplayStats();

        //return 0;
    };
};