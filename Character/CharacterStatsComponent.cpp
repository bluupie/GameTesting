#include "CharacterStatsComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UCharacterStatsComponent::UCharacterStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UCharacterStatsComponent::BeginPlay()
{
    Super::BeginPlay();
    if (CanMutate()) RecalculateStats(EResourceRecalculationPolicy::FillToMaximum);
}

void UCharacterStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCharacterStatsComponent, Progression);
    DOREPLIFETIME(UCharacterStatsComponent, BaseAttributes);
    DOREPLIFETIME(UCharacterStatsComponent, CalculatedAttributes);
    DOREPLIFETIME(UCharacterStatsComponent, CalculatedStats);
    DOREPLIFETIME(UCharacterStatsComponent, StatsRevision);
}

bool UCharacterStatsComponent::CanMutate() const
{
    const AActor* Owner = GetOwner();
    return !Owner || Owner->HasAuthority();
}

bool UCharacterStatsComponent::SetBaseAttributes(const FBaseAttributes& NewBaseAttributes,
                                                  EResourceRecalculationPolicy ResourcePolicy)
{
    if (!CanMutate()) return false;
    BaseAttributes = NewBaseAttributes;
    RecalculateStats(ResourcePolicy);
    return true;
}

bool UCharacterStatsComponent::SetCharacterLevel(int32 NewLevel, EResourceRecalculationPolicy ResourcePolicy)
{
    if (!CanMutate()) return false;
    Progression.CurrentLevel = FMath::Clamp(NewLevel, 1, FMath::Max(1, Progression.MaximumLevel));
    RecalculateStats(ResourcePolicy);
    return true;
}

bool UCharacterStatsComponent::UpsertStatSource(const FCharacterStatSource& Source,
                                                 EResourceRecalculationPolicy ResourcePolicy)
{
    if (!CanMutate() || Source.SourceId.IsNone()) return false;
    StatSources.Add(Source.SourceId, Source);
    RecalculateStats(ResourcePolicy);
    return true;
}

bool UCharacterStatsComponent::RemoveStatSource(FName SourceId, EResourceRecalculationPolicy ResourcePolicy)
{
    if (!CanMutate() || SourceId.IsNone() || StatSources.Remove(SourceId) == 0) return false;
    RecalculateStats(ResourcePolicy);
    return true;
}

int32 UCharacterStatsComponent::RemoveStatSourcesByType(ECharacterStatSourceType SourceType,
                                                         EResourceRecalculationPolicy ResourcePolicy)
{
    if (!CanMutate()) return 0;
    TArray<FName> KeysToRemove;
    for (const TPair<FName, FCharacterStatSource>& Pair : StatSources)
    {
        if (Pair.Value.SourceType == SourceType) KeysToRemove.Add(Pair.Key);
    }
    for (FName Key : KeysToRemove) StatSources.Remove(Key);
    if (KeysToRemove.Num() > 0) RecalculateStats(ResourcePolicy);
    return KeysToRemove.Num();
}

bool UCharacterStatsComponent::SetStatSourceEnabled(FName SourceId, bool bEnabled,
                                                     EResourceRecalculationPolicy ResourcePolicy)
{
    if (!CanMutate()) return false;
    FCharacterStatSource* Source = StatSources.Find(SourceId);
    if (!Source || Source->bEnabled == bEnabled) return false;
    Source->bEnabled = bEnabled;
    RecalculateStats(ResourcePolicy);
    return true;
}

bool UCharacterStatsComponent::SetGearContribution(FName SourceId, const FGearStatContribution& Contribution,
                                                    EResourceRecalculationPolicy ResourcePolicy)
{
    FCharacterStatSource Source;
    Source.SourceId = SourceId;
    Source.SourceType = ECharacterStatSourceType::Equipment;
    Source.StatPool = Contribution.StatPool;
    Source.BonusStrength = FMath::RoundToInt(Contribution.BonusStrength);
    Source.BonusDexterity = FMath::RoundToInt(Contribution.BonusDexterity);
    Source.BonusIntelligence = FMath::RoundToInt(Contribution.BonusIntelligence);
    return UpsertStatSource(Source, ResourcePolicy);
}

bool UCharacterStatsComponent::SetPassiveContribution(FName SourceId, const FPassiveStatContribution& Contribution,
                                                       EResourceRecalculationPolicy ResourcePolicy)
{
    FCharacterStatSource Source;
    Source.SourceId = SourceId;
    Source.SourceType = ECharacterStatSourceType::PassiveTree;
    Source.StatPool = Contribution.StatPool;
    Source.BonusStrength = Contribution.BonusStrength;
    Source.BonusDexterity = Contribution.BonusDexterity;
    Source.BonusIntelligence = Contribution.BonusIntelligence;
    return UpsertStatSource(Source, ResourcePolicy);
}

FModifierPool UCharacterStatsComponent::BuildCombinedModifierPool() const
{
    FModifierPool Combined;
    for (const TPair<FName, FCharacterStatSource>& SourcePair : StatSources)
    {
        if (!SourcePair.Value.bEnabled) continue;
        for (const TPair<EStatType, FStatModifier>& ModPair : SourcePair.Value.StatPool.Mods)
        {
            FStatModifier& Target = Combined.Mods.FindOrAdd(ModPair.Key);
            Target.Flat += ModPair.Value.Flat;
            Target.Percent += ModPair.Value.Percent;
            Target.MorePercent *= ModPair.Value.MorePercent;
        }
    }
    return Combined;
}

void UCharacterStatsComponent::RecalculateStats(EResourceRecalculationPolicy ResourcePolicy)
{
    if (!CanMutate()) return;

    const bool bWasCalculated = CalculatedStats.bHasCalculatedStats;
    const float OldHealthRatio = CalculatedStats.MaximumHealth > 0.0f
        ? CalculatedStats.CurrentHealth / CalculatedStats.MaximumHealth : 1.0f;
    const float OldManaRatio = CalculatedStats.MaximumMana > 0.0f
        ? CalculatedStats.CurrentMana / CalculatedStats.MaximumMana : 1.0f;

    CalculatedAttributes = BaseAttributes;
    for (const TPair<FName, FCharacterStatSource>& Pair : StatSources)
    {
        if (!Pair.Value.bEnabled) continue;
        CalculatedAttributes.Strength += Pair.Value.BonusStrength;
        CalculatedAttributes.Dexterity += Pair.Value.BonusDexterity;
        CalculatedAttributes.Intelligence += Pair.Value.BonusIntelligence;
    }

    CalculatedStats.Recalculate(CalculatedAttributes, BuildCombinedModifierPool(), Progression.CurrentLevel);

    if (bWasCalculated)
    {
        switch (ResourcePolicy)
        {
            case EResourceRecalculationPolicy::PreservePercentage:
                CalculatedStats.CurrentHealth = FMath::Clamp(OldHealthRatio, 0.0f, 1.0f) * CalculatedStats.MaximumHealth;
                CalculatedStats.CurrentMana = FMath::Clamp(OldManaRatio, 0.0f, 1.0f) * CalculatedStats.MaximumMana;
                break;
            case EResourceRecalculationPolicy::FillToMaximum:
                CalculatedStats.CurrentHealth = CalculatedStats.MaximumHealth;
                CalculatedStats.CurrentMana = CalculatedStats.MaximumMana;
                break;
            case EResourceRecalculationPolicy::PreserveCurrent:
            default:
                break;
        }
    }

    ++StatsRevision;
    OnStatsRecalculated.Broadcast();
}

void UCharacterStatsComponent::OnRep_StatsData()
{
    OnStatsRecalculated.Broadcast();
}

float UCharacterStatsComponent::GetCalculatedStat(EStatType StatType) const
{
    switch (StatType)
    {
        case EStatType::Life:                     return CalculatedStats.MaximumHealth;
        case EStatType::Mana:                     return CalculatedStats.MaximumMana;
        case EStatType::Barrier:                  return CalculatedStats.Barrier;
        case EStatType::DamagePhysical:           return CalculatedStats.DamagePhysical;
        case EStatType::DamageFire:               return CalculatedStats.DamageFire;
        case EStatType::DamageCold:               return CalculatedStats.DamageCold;
        case EStatType::DamageLightning:          return CalculatedStats.DamageLightning;
        case EStatType::DamagePoison:             return CalculatedStats.DamagePoison;
        case EStatType::CriticalStrikeChance:     return CalculatedStats.CriticalStrikeChance;
        case EStatType::CriticalStrikeMultiplier: return CalculatedStats.CriticalStrikeMultiplier;
        case EStatType::AttackSpeed:              return CalculatedStats.AttackSpeed;
        case EStatType::MovementSpeed:            return CalculatedStats.MovementSpeed;
        case EStatType::Armour:                   return CalculatedStats.Armour;
        case EStatType::FireResistance:           return CalculatedStats.FireResistance;
        case EStatType::ColdResistance:           return CalculatedStats.ColdResistance;
        case EStatType::LightningResistance:      return CalculatedStats.LightningResistance;
        case EStatType::PoisonResistance:         return CalculatedStats.PoisonResistance;
        case EStatType::Evasion:                  return CalculatedStats.Evasion;
        case EStatType::BlockChance:              return CalculatedStats.BlockChance;
        case EStatType::BlockAmount:              return CalculatedStats.BlockAmount;
        case EStatType::LifeLeech:                return CalculatedStats.LifeLeech;
        case EStatType::ManaLeech:                return CalculatedStats.ManaLeech;
        case EStatType::EnergyShield:             return CalculatedStats.EnergyShield;
        case EStatType::Accuracy:                 return CalculatedStats.Accuracy;
        case EStatType::DodgeChance:              return CalculatedStats.DodgeChance;
        case EStatType::SpellBlock:               return CalculatedStats.SpellBlockChance;
        case EStatType::SpellDamage:              return CalculatedStats.SpellDamage;
        case EStatType::CastSpeed:                return CalculatedStats.CastSpeed;
        case EStatType::CooldownReduction:        return CalculatedStats.CooldownReduction;
        case EStatType::HealthRegen:              return CalculatedStats.LifeRegeneration;
        case EStatType::ManaRegen:                return CalculatedStats.ManaRegeneration;
        case EStatType::AoeRadius:                return CalculatedStats.AoeRadius;
        case EStatType::DamageWithSwords:         return CalculatedStats.DamageWithSwords;
        case EStatType::DamageWithAxes:           return CalculatedStats.DamageWithAxes;
        case EStatType::ProjectileDamage:         return CalculatedStats.ProjectileDamage;
        case EStatType::AdditionalProjectiles:    return static_cast<float>(CalculatedStats.AdditionalProjectiles);
        default:                                  return 0.0f;
    }
}
