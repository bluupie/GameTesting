#include <iostream>
#include <math>
#include <string>
#include <iomanip>

struct LevelUpGrowthNodifiers{
    float maximumHealthModifier;
    float maximumManaModifier;
    float attackDaamageModifier;
    float defenceModifier;
};

class Character {
    private:
        std::string name;
        std::string className;
        int currentLevel;
        int maximumLevel;
        int currentExp;
        int expRequired;

        //Base Stats
        int baseMaximumHealth;
        int baseMaximumMana;
        int baseAttackDamage;
        int baseDefence;

        // Current Stats
        int maximumHealth;
        int currentHealth;
        int maximumMana;
        int currentMana;
        int attackDamage;
        int defence;

        LevelUpGrowthNodifiers growth;

        int calculateRequiredExp(int currentLevel){
            if(currentLevel <= 1) return 0;
            return static_cast<int>(std::round((100.0f * std::pow(currentLevel, 3)) / 3.0f);
        };
        
        void UpdateStats() {
            float levelFactor = std::pow(currentLevel - 1, 1.2f);

            maximumHealth = baseMaximumHealth = static_cast<int>(growth.maximumHealthModifier * levelFactor);
            maximumMana = baseMaximumMana = static_cast<int>(growth.maximumManaModifier * levelFactor);
            attackDamage = baseAttackDamage = static_cast<int>(growth.attackDaamageModifier * levelFactor);
            defence = baseDefence = static_cast<int>(growth.defenceModifier * levelFactor);

            expRequired = calculateRequiredExp(currentLevel + 1) - calculateRequiredExp(currentLevel);
        }
}