#include <string>
#include <memory>
#include <random>
#include <vector>
#include <algorithm>
#include <iostream>

// ==========================================
// 1. WEAPON MODIFIERS
// ==========================================

enum class PhysicalWeaponPrefixModifiers {
    // Can roll on all weapon types   
    IncreasedPhysicalDamagePercent,
    PhysicalDamageToAttacks,
    FireDamageToAttacks,
    ColdDamageToAttacks,
    LightningDamageToAttacks,
    PoisonDamageToAttacks,
    ElementalDamageWithAttacks,
    DamageOverTimePercent,
    FireDamagePercent,
    ColdDamagePercent,
    LightningDamagePercent,
    PoisonDamagePercent,
    PrimaryResource,
    Accuracy
};

enum class PhysicalWeaponSuffixModifiers {
    AttackSpeed,
    CriticalStrikeChance,
    CriticalStrikeMultiplier,
    IncreasedStrength,
    IncreasedDexterity,
    IncreasedIntelligence,
    LifeGainOnHit,
    PrimaryResourceGainOnHit
};

enum class CasterWeaponPrefixModifiers {
    IncreasedSpellDamage,
    FireDamageToSpells,
    ColdDamageToSpells,
    LightningDamageToSpells,
    FireDamagePercent,
    ColdDamagePercent,
    LightningDamagePercent,
    PoisonDamagePercent,
    ElementalDamagePercent,
    DamageOverTimePercent,
    PrimaryResource
};

enum class CasterWeaponSuffixModifiers {
    CastSpeed,
    CriticalStrikeChanceWithSpells,
    CriticalStrikeMultiplier,
    PrimaryResourceGainOnHit,
    IncreasedIntelligence
};

// ==========================================
// 2. ARMOUR (ARMOUR-BASED) MODIFIERS
// ==========================================

enum class ArmourGlovesPrefixModifier {
    IncreasedArmourPercent,
    IncreasedArmourFlat,
    IncreasedMaximumLife,
    PrimaryResource,
    FireDamageToAttacks,
    ColdDamageToAttacks,
    LightningDamageToAttacks,
    PhysicalDamageToAttacks,
    Accuracy
};

enum class ArmourArmourPrefixModifier {
    IncreasedArmourPercent,
    IncreasedArmourFlat,
    IncreasedMaximumLife,
    PrimaryResource
};

enum class ArmourBootsPrefixModifiers {
    IncreasedArmourFlat,
    IncreasedArmourPercent,
    MovementSpeed,
    IncreasedMaximumLife,
    PrimaryResource
};

enum class ArmourSuffixModifiers {
    IncreasedStrength,
    LifeRegeneration,
    FireResistance,
    ColdResistance,
    LightningResistance,
    PoisonResistance
};

// ==========================================
// 3. ENERGY SHIELD MODIFIERS
// ==========================================

enum class EnergyShieldGlovesPrefixModifier {
    IncreasedEnergyShieldPercent,
    IncreasedEnergyShieldFlat,
    IncreasedMaximumLife,
    PrimaryResource,
    FireDamageToSpells,
    ColdDamageToSpells,
    LightningDamageToSpells,
    PhysicalDamageToSpells
};

enum class EnergyShieldArmourPrefixModifiers {
    IncreasedEnergyShieldPercent,
    IncreasedEnergyShieldFlat,
    IncreasedMaximumLife,
    PrimaryResource
};

enum class EnergyShieldBootsPrefixModifiers {
    IncreasedEnergyShieldFlat,
    IncreasedEnergyShieldPercent,
    MovementSpeed,
    IncreasedMaximumLife,
    PrimaryResource
};

enum class EnergyShieldSuffixModifiers {
    EnergyShieldRecharge,
    IncreasedIntelligence,
    FireResistance,
    ColdResistance,
    LightningResistance,
    PoisonResistance
};

// ==========================================
// 4. EVASION MODIFIERS
// ==========================================

enum class EvasionGlovesPrefixModifier {
    IncreasedEvasionPercent,
    IncreasedEvasionFlat,
    IncreasedMaximumLife,
    PrimaryResource,
    FireDamageToAttacks,
    ColdDamageToAttacks,
    LightningDamageToAttacks,
    PhysicalDamageToAttacks,
    Accuracy
};

enum class EvasionArmourPrefixModifiers {
    IncreasedEvasionPercent,
    IncreasedEvasionFlat,
    IncreasedMaximumLife,
    PrimaryResource
};

enum class EvasionBootsPrefixModifiers {
    IncreasedEvasionFlat,
    IncreasedEvasionPercent,
    MovementSpeed,
    IncreasedMaximumLife,
    PrimaryResource
};

enum class EvasionSuffixModifiers {
    LifeRegeneration,
    IncreasedDexterity,
    FireResistance,
    ColdResistance,
    LightningResistance,
    PoisonResistance
};

// ==========================================
// 5. JEWELLERY & BELT MODIFIERS
// ==========================================

enum class JewelleryPrefixModifiers {
    IncreasedMaximumLife,
    PrimaryResource,
    IncreasedArmourFlat,
    IncreasedEvasionFlat,
    IncreasedEnergyShieldFlat,
    FireDamageToAttacks,
    ColdDamageToAttacks,
    LightningDamageToAttacks,
    FireDamageToSpells,
    ColdDamageToSpells,
    LightningDamageToSpells,
    PhysicalDamageToAttacks,
    PhysicalDamageToSpells,
    ElementalDamageWithAttacks,
    ElementalDamagePercent
};

enum class JewellerySuffixModifiers {
    IncreasedDexterity,
    IncreasedStrength,
    IncreasedIntelligence,
    IncreasedAllAttributes,
    CastSpeed,
    AttackSpeed,
    LifeRegeneration,
    PrimaryResourceGainOnHit,
    PrimaryResource,
    FireResistance,
    ColdResistance,
    LightningResistance,
    PoisonResistance,
    AllResistances,
    FireDamagePercent,
    ColdDamagePercent,
    LightningDamagePercent,
    PoisonDamagePercent,
    Accuracy
};

enum class BeltPrefixModifiers {
    IncreasedMaximumLife,
    PrimaryResource,
    IncreasedArmourFlat,
    IncreasedEvasionFlat,
    IncreasedEnergyShieldFlat,
    ElementalDamageWithAttacks,
    ElementalDamagePercent
};

enum class BeltSuffixModifiers {
    IncreasedStrength,
    IncreasedDexterity,
    IncreasedIntelligence,
    LifeRegeneration,
    ColdResistance,
    FireResistance,
    LightningResistance,
    PoisonResistance
};

// ==========================================
// 6. CORE AFFIX TYPES & CLASSES
// ==========================================

enum class ModifierScaleType {
    Flat,
    Percentage,
    More
};

enum class ModifierType {
    Prefix,
    Suffix
};

struct AffixTemplate {
    std::string id;
    std::string name; // e.g., "Squire's" or "of the Drake"
    ModifierType type;
    
    // In your original code, 'Stat' was of type 'Modifier', which matches the template class name.
    // Assuming you intended an integer identifier, string, or enum mapping for the stat type.
    int stat; 
    
    ModifierScaleType modType;
    float minValue;
    float maxValue;
    int itemLevelRequired;
};

class AffixInstance {
private:
    std::shared_ptr<AffixTemplate> templateRef;
    float rolledValue;

public:
    // Constructor handles the random generation bound by min/max boundaries
    explicit AffixInstance(std::shared_ptr<AffixTemplate> t) : templateRef(t) {
        // Use standard modern C++ random number engine
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(templateRef->minValue, templateRef->maxValue);
        
        rolledValue = dis(gen);
    }

    // Getters to replicate C# public read-only properties
    [[nodiscard]] std::shared_ptr<AffixTemplate> getTemplate() const { return templateRef; }
    [[nodiscard]] float getRolledValue() const { return rolledValue; }
};
// ==========================================
// 7. ITEM CLASS
// ==========================================
class Item {
public:
    std::string name;
    std::vector<AffixInstance> prefixes;
    std::vector<AffixInstance> suffixes;

    void display() const {
        std::cout << "--- Item: " << name << " ---\n";
        std::cout << "[Prefixes]:\n";
        for (const auto& affix : prefixes) {
            std::cout << "  - " << affix.getTemplate()->name << ": " << affix.getRolledValue() << "\n";
        }
        std::cout << "[Suffixes]:\n";
        for (const auto& affix : suffixes) {
            std::cout << "  - " << affix.getTemplate()->name << ": " << affix.getRolledValue() << "\n";
        }
        std::cout << "----------------------\n\n";
    }
};

// ==========================================
// 8. ITEM GENERATOR LOGIC
// ==========================================
class ItemGenerator {
public:
    // This function accepts a massive pool of all your game's available templates
    static Item generateSixAffixItem(const std::vector<std::shared_ptr<AffixTemplate>>& globalTemplatePool, const std::string& itemName) {
        Item newItem;
        newItem.name = itemName;

        // 1. Separate the global pool into Prefixes and Suffixes
        std::vector<std::shared_ptr<AffixTemplate>> availablePrefixes;
        std::vector<std::shared_ptr<AffixTemplate>> availableSuffixes;

        for (const auto& t : globalTemplatePool) {
            if (t->type == ModifierType::Prefix) {
                availablePrefixes.push_back(t);
            } else if (t->type == ModifierType::Suffix) {
                availableSuffixes.push_back(t);
            }
        }

        // 2. Setup standard random engine
        std::random_device rd;
        std::mt19937 gen(rd());

        // 3. Shuffle the pools randomly
        std::shuffle(availablePrefixes.begin(), availablePrefixes.end(), gen);
        std::shuffle(availableSuffixes.begin(), availableSuffixes.end(), gen);

        // 4. Take up to 3 elements from the shuffled prefix pool and roll them
        size_t prefixesToTake = std::min<size_t>(3, availablePrefixes.size());
        for (size_t i = 0; i < prefixesToTake; ++i) {
            newItem.prefixes.push_back(AffixInstance(availablePrefixes[i]));
        }

        // 5. Take up to 3 elements from the shuffled suffix pool and roll them
        size_t suffixesToTake = std::min<size_t>(3, availableSuffixes.size());
        for (size_t i = 0; i < suffixesToTake; ++i) {
            newItem.suffixes.push_back(AffixInstance(availableSuffixes[i]));
        }

        return newItem;
    }
};

int main() {
    // 1. Populate your master game database with templates mapping to your enums
    std::vector<std::shared_ptr<AffixTemplate>> gameDatabase = {
        // Prefixes
        std::make_shared<AffixTemplate>("p1", "Squire's", ModifierType::Prefix, static_cast<int>(PhysicalWeaponPrefixModifiers::IncreasedPhysicalDamagePercent), ModifierScaleType::Percentage, 10.0f, 20.0f, 1),
        std::make_shared<AffixTemplate>("p2", "Heavy", ModifierType::Prefix, static_cast<int>(PhysicalWeaponPrefixModifiers::PhysicalDamageToAttacks), ModifierScaleType::Flat, 5.0f, 15.0f, 1),
        std::make_shared<AffixTemplate>("p3", "Glinting", ModifierType::Prefix, static_cast<int>(PhysicalWeaponPrefixModifiers::LightningDamageToAttacks), ModifierScaleType::Flat, 1.0f, 30.0f, 5),
        std::make_shared<AffixTemplate>("p4", "Archmage's", ModifierType::Prefix, static_cast<int>(CasterWeaponPrefixModifiers::IncreasedSpellDamage), ModifierScaleType::Percentage, 40.0f, 70.0f, 10),
        std::make_shared<AffixTemplate>("p5", "Titan's", ModifierType::Prefix, static_cast<int>(ArmourArmourPrefixModifier::IncreasedArmourFlat), ModifierScaleType::Flat, 10.0F, 20.0F, 1),

        // Suffixes
        std::make_shared<AffixTemplate>("s1", "of the Drake", ModifierType::Suffix, static_cast<int>(PhysicalWeaponSuffixModifiers::CriticalStrikeChance), ModifierScaleType::Flat, 5.0f, 10.0f, 1),
        std::make_shared<AffixTemplate>("s2", "of the Cheetah", ModifierType::Suffix, static_cast<int>(PhysicalWeaponSuffixModifiers::AttackSpeed), ModifierScaleType::Percentage, 3.0f, 12.0f, 1),
        std::make_shared<AffixTemplate>("s3", "of the Fox", ModifierType::Suffix, static_cast<int>(PhysicalWeaponSuffixModifiers::IncreasedDexterity), ModifierScaleType::Flat, 10.0f, 20.0f, 3),
        std::make_shared<AffixTemplate>("s4", "of the Sage", ModifierType::Suffix, static_cast<int>(CasterWeaponSuffixModifiers::CastSpeed), ModifierScaleType::Percentage, 5.0f, 15.0f, 5)
    };

    // 2. Generate an item drawing exactly 3 prefixes and 3 suffixes
    Item craftedSword = ItemGenerator::generateSixAffixItem(gameDatabase, "Gilded Broadsword");

    // 3. Print out the results!
    craftedSword.display();

    return 0;
}
