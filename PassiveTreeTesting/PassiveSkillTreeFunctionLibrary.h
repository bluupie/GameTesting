#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PassiveSkillTreeTypes.h"
#include "PassiveSkillTreeFunctionLibrary.generated.h"

UCLASS()
class UPassiveSkillTreeFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Passive Skill Tree")
    static FPassiveSkillTreeState CreateInitialState(EPassiveAttributeSection StartingSection, int32 AvailablePoints = 0);

    UFUNCTION(BlueprintPure, Category = "Passive Skill Tree")
    static EPassiveAllocationResult CanAllocateNode(const FPassiveSkillTreeDefinition& Tree,
                                                     const FPassiveSkillTreeState& State, FName NodeId);

    UFUNCTION(BlueprintCallable, Category = "Passive Skill Tree")
    static FPassiveAllocationOutcome AllocateNode(const FPassiveSkillTreeDefinition& Tree,
                                                   UPARAM(ref) FPassiveSkillTreeState& State, FName NodeId);

    UFUNCTION(BlueprintPure, Category = "Passive Skill Tree")
    static EPassiveAllocationResult CanRefundNode(const FPassiveSkillTreeDefinition& Tree,
                                                   const FPassiveSkillTreeState& State, FName NodeId);

    UFUNCTION(BlueprintCallable, Category = "Passive Skill Tree")
    static FPassiveAllocationOutcome RefundNode(const FPassiveSkillTreeDefinition& Tree,
                                                 UPARAM(ref) FPassiveSkillTreeState& State, FName NodeId);

    UFUNCTION(BlueprintPure, Category = "Passive Skill Tree")
    static TArray<FName> GetAllocatableNodes(const FPassiveSkillTreeDefinition& Tree,
                                              const FPassiveSkillTreeState& State);

    UFUNCTION(BlueprintPure, Category = "Passive Skill Tree")
    static FPassiveStatContribution BuildPassiveContribution(const FPassiveSkillTreeDefinition& Tree,
                                                               const FPassiveSkillTreeState& State);

    UFUNCTION(BlueprintPure, Category = "Passive Skill Tree")
    static FBaseAttributes ApplyPassiveAttributes(const FBaseAttributes& BaseAttributes,
                                                   const FPassiveStatContribution& Contribution);

    UFUNCTION(BlueprintCallable, Category = "Passive Skill Tree")
    static void MergeModifierPool(UPARAM(ref) FModifierPool& Target, const FModifierPool& Source);

    UFUNCTION(BlueprintPure, Category = "Passive Skill Tree")
    static bool ValidateTree(const FPassiveSkillTreeDefinition& Tree, TArray<FString>& OutErrors);

    UFUNCTION(BlueprintCallable, Category = "Passive Skill Tree")
    static FPassiveSkillTreeDefinition GetExampleThreeSectionTree();

private:
    static const FPassiveSkillNode* FindNode(const FPassiveSkillTreeDefinition& Tree, FName NodeId);
    static bool AreConnected(const FPassiveSkillTreeDefinition& Tree, FName A, FName B);
    static bool AllAllocatedNodesReachAStart(const FPassiveSkillTreeDefinition& Tree,
                                             const FPassiveSkillTreeState& State,
                                             const TSet<FName>& AllocationSet);
};
