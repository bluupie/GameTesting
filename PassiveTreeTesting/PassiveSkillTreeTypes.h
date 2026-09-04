#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CharacterStatsTypes.h"
#include "PassiveSkillTreeTypes.generated.h"

UENUM(BlueprintType)
enum class EPassiveAttributeSection : uint8
{
    Strength,
    Intelligence,
    Dexterity
};

UENUM(BlueprintType)
enum class EPassiveNodeType : uint8
{
    Start,
    Minor,
    Notable,
    Keystone
};

UENUM(BlueprintType)
enum class EPassiveEffectTarget : uint8
{
    Stat,
    Attribute
};

UENUM(BlueprintType)
enum class EPassiveAttributeType : uint8
{
    Strength,
    Dexterity,
    Intelligence
};

UENUM(BlueprintType)
enum class EPassiveModifierApplication : uint8
{
    Flat,
    Increased,
    More
};

UENUM(BlueprintType)
enum class EPassiveAllocationResult : uint8
{
    Success,
    Failed_NodeNotFound,
    Failed_AlreadyAllocated,
    Failed_NotAllocated,
    Failed_NoPoints,
    Failed_NotConnected,
    Failed_StartNodeLocked,
    Failed_StartNodeCannotBeRefunded,
    Failed_WouldDisconnectTree
};

USTRUCT(BlueprintType)
struct FPassiveNodeEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    EPassiveEffectTarget Target = EPassiveEffectTarget::Stat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive", meta = (EditCondition = "Target == EPassiveEffectTarget::Stat"))
    EStatType StatType = EStatType::Life;

    // Applies the same value to several stats. Useful for Elemental Damage,
    // which targets Fire, Cold and Lightning from one passive effect.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive", meta = (EditCondition = "Target == EPassiveEffectTarget::Stat"))
    TArray<EStatType> AdditionalStatTypes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive", meta = (EditCondition = "Target == EPassiveEffectTarget::Stat"))
    EPassiveModifierApplication Application = EPassiveModifierApplication::Flat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive", meta = (EditCondition = "Target == EPassiveEffectTarget::Attribute"))
    EPassiveAttributeType AttributeType = EPassiveAttributeType::Strength;

    // Percent values use fractions: 0.10 means 10%.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FPassiveSkillNode : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    FName NodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive", meta = (MultiLine = true))
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    EPassiveAttributeSection Section = EPassiveAttributeSection::Strength;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    EPassiveNodeType NodeType = EPassiveNodeType::Minor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive", meta = (ClampMin = "0"))
    int32 PointCost = 1;

    // Coordinates consumed by UMG; the rules system does not depend on layout.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    FVector2D TreePosition = FVector2D::ZeroVector;

    // Connections are treated as undirected, even if only one endpoint lists
    // the other. Authoring both directions is still recommended for clarity.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    TArray<FName> ConnectedNodeIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    TArray<FPassiveNodeEffect> Effects;
};

USTRUCT(BlueprintType)
struct FPassiveSkillTreeDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    TArray<FPassiveSkillNode> Nodes;
};

USTRUCT(BlueprintType)
struct FPassiveSkillTreeState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    int32 AvailablePoints = 0;

    // Determines which class/archetype start node can be allocated for free.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    TArray<FName> ActiveStartNodeIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
    TSet<FName> AllocatedNodeIds;
};

USTRUCT(BlueprintType)
struct FPassiveAllocationOutcome
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Passive")
    EPassiveAllocationResult Result = EPassiveAllocationResult::Failed_NodeNotFound;

    UPROPERTY(BlueprintReadOnly, Category = "Passive")
    int32 RemainingPoints = 0;
};

USTRUCT(BlueprintType)
struct FPassiveStatContribution
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Passive")
    FModifierPool StatPool;

    UPROPERTY(BlueprintReadOnly, Category = "Passive")
    int32 BonusStrength = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Passive")
    int32 BonusDexterity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Passive")
    int32 BonusIntelligence = 0;
};
