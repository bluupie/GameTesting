#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterStatsTypes.h"
#include "Gear/GearAffixTypes.h"
#include "PassiveSkillTreeTypes.h"
#include "CharacterStatsComponent.generated.h"

UENUM(BlueprintType)
enum class ECharacterStatSourceType : uint8
{
    Equipment,
    PassiveTree,
    Buff,
    Debuff,
    Other
};

UENUM(BlueprintType)
enum class EResourceRecalculationPolicy : uint8
{
    PreserveCurrent,
    PreservePercentage,
    FillToMaximum
};

// One independently replaceable contribution to the character. Stable source
// IDs prevent double-applying the same item, passive tree, aura, or buff.
USTRUCT(BlueprintType)
struct FCharacterStatSource
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
    FName SourceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
    ECharacterStatSourceType SourceType = ECharacterStatSourceType::Other;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
    FModifierPool StatPool;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
    int32 BonusStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
    int32 BonusDexterity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
    int32 BonusIntelligence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
    bool bEnabled = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterStatsRecalculated);

UCLASS(ClassGroup = (Character), meta = (BlueprintSpawnableComponent))
class UCharacterStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCharacterStatsComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, SaveGame, Category = "Stats|Character")
    FCharacterProgression Progression;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, SaveGame, Category = "Stats|Character")
    FBaseAttributes BaseAttributes;

    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Stats|Calculated")
    FBaseAttributes CalculatedAttributes;

    UPROPERTY(BlueprintReadOnly, Replicated, SaveGame, Category = "Stats|Calculated")
    FCharacterStats CalculatedStats;

    // Source data is authoritative/save data. Clients receive the calculated
    // result and do not need every internal modifier replicated to them.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Stats|Sources")
    TMap<FName, FCharacterStatSource> StatSources;

    UPROPERTY(BlueprintAssignable, Category = "Stats")
    FOnCharacterStatsRecalculated OnStatsRecalculated;

    UFUNCTION(BlueprintCallable, Category = "Stats|Character")
    bool SetBaseAttributes(const FBaseAttributes& NewBaseAttributes,
                           EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Stats|Character")
    bool SetCharacterLevel(int32 NewLevel,
                           EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreservePercentage);

    UFUNCTION(BlueprintCallable, Category = "Stats|Sources")
    bool UpsertStatSource(const FCharacterStatSource& Source,
                          EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Stats|Sources")
    bool RemoveStatSource(FName SourceId,
                          EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Stats|Sources")
    int32 RemoveStatSourcesByType(ECharacterStatSourceType SourceType,
                                  EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Stats|Sources")
    bool SetStatSourceEnabled(FName SourceId, bool bEnabled,
                              EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Stats|Sources")
    bool SetGearContribution(FName SourceId, const FGearStatContribution& Contribution,
                             EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Stats|Sources")
    bool SetPassiveContribution(FName SourceId, const FPassiveStatContribution& Contribution,
                                EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stats")
    void RecalculateStats(EResourceRecalculationPolicy ResourcePolicy = EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintPure, Category = "Stats")
    float GetCalculatedStat(EStatType StatType) const;

    UFUNCTION(BlueprintPure, Category = "Stats")
    bool HasStatSource(FName SourceId) const { return StatSources.Contains(SourceId); }

private:
    // A single replicated notification avoids firing the delegate once for
    // every replicated struct in the same server update.
    UPROPERTY(ReplicatedUsing = OnRep_StatsData)
    int32 StatsRevision = 0;

    bool CanMutate() const;
    FModifierPool BuildCombinedModifierPool() const;

    UFUNCTION()
    void OnRep_StatsData();
};
