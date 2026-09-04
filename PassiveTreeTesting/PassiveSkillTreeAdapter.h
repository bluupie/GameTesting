#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterStatsComponent.h"
#include "PassiveSkillTreeFunctionLibrary.h"
#include "PassiveSkillTreeAdapter.generated.h"

UCLASS()
class UPassiveSkillTreeAdapter : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Passive Skill Tree|Integration")
    static bool RefreshPassiveStats(UCharacterStatsComponent* StatsComponent,
                                    const FPassiveSkillTreeDefinition& Tree,
                                    const FPassiveSkillTreeState& State,
                                    EResourceRecalculationPolicy ResourcePolicy =
                                        EResourceRecalculationPolicy::PreserveCurrent);

    UFUNCTION(BlueprintCallable, Category = "Passive Skill Tree|Integration")
    static FPassiveAllocationOutcome AllocateNodeAndRefreshStats(
        UCharacterStatsComponent* StatsComponent,
        const FPassiveSkillTreeDefinition& Tree,
        UPARAM(ref) FPassiveSkillTreeState& State,
        FName NodeId);

    UFUNCTION(BlueprintCallable, Category = "Passive Skill Tree|Integration")
    static FPassiveAllocationOutcome RefundNodeAndRefreshStats(
        UCharacterStatsComponent* StatsComponent,
        const FPassiveSkillTreeDefinition& Tree,
        UPARAM(ref) FPassiveSkillTreeState& State,
        FName NodeId);

private:
    static const FName PassiveTreeSourceId;
};
