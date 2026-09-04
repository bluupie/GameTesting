#include "PassiveSkillTreeAdapter.h"

const FName UPassiveSkillTreeAdapter::PassiveTreeSourceId(TEXT("PassiveTree"));

bool UPassiveSkillTreeAdapter::RefreshPassiveStats(
    UCharacterStatsComponent* StatsComponent,
    const FPassiveSkillTreeDefinition& Tree,
    const FPassiveSkillTreeState& State,
    EResourceRecalculationPolicy ResourcePolicy)
{
    if (!StatsComponent || !StatsComponent->IsStatMutationAuthority())
    {
        return false;
    }

    const FPassiveStatContribution Contribution =
        UPassiveSkillTreeFunctionLibrary::BuildPassiveContribution(Tree, State);
    return StatsComponent->SetPassiveContribution(
        PassiveTreeSourceId, Contribution, ResourcePolicy);
}

FPassiveAllocationOutcome UPassiveSkillTreeAdapter::AllocateNodeAndRefreshStats(
    UCharacterStatsComponent* StatsComponent,
    const FPassiveSkillTreeDefinition& Tree,
    FPassiveSkillTreeState& State,
    FName NodeId)
{
    FPassiveAllocationOutcome Outcome;
    Outcome.RemainingPoints = State.AvailablePoints;

    if (!StatsComponent)
    {
        Outcome.Result = EPassiveAllocationResult::Failed_StatsComponentUnavailable;
        return Outcome;
    }
    if (!StatsComponent->IsStatMutationAuthority())
    {
        Outcome.Result = EPassiveAllocationResult::Failed_NotAuthority;
        return Outcome;
    }

    FPassiveSkillTreeState PreparedState = State;
    Outcome = UPassiveSkillTreeFunctionLibrary::AllocateNode(
        Tree, PreparedState, NodeId);
    if (Outcome.Result != EPassiveAllocationResult::Success)
    {
        return Outcome;
    }

    if (const FCharacterStatSource* Existing = StatsComponent->StatSources.Find(PassiveTreeSourceId))
    {
        if (Existing->SourceType != ECharacterStatSourceType::PassiveTree)
        {
            Outcome.Result = EPassiveAllocationResult::Failed_StatRefresh;
            Outcome.RemainingPoints = State.AvailablePoints;
            return Outcome;
        }
    }

    const FPassiveSkillTreeState OriginalState = State;
    State = MoveTemp(PreparedState);
    if (!RefreshPassiveStats(StatsComponent, Tree, State,
                             EResourceRecalculationPolicy::PreserveCurrent))
    {
        State = OriginalState;
        Outcome.Result = EPassiveAllocationResult::Failed_StatRefresh;
        Outcome.RemainingPoints = State.AvailablePoints;
        return Outcome;
    }
    return Outcome;
}

FPassiveAllocationOutcome UPassiveSkillTreeAdapter::RefundNodeAndRefreshStats(
    UCharacterStatsComponent* StatsComponent,
    const FPassiveSkillTreeDefinition& Tree,
    FPassiveSkillTreeState& State,
    FName NodeId)
{
    FPassiveAllocationOutcome Outcome;
    Outcome.RemainingPoints = State.AvailablePoints;

    if (!StatsComponent)
    {
        Outcome.Result = EPassiveAllocationResult::Failed_StatsComponentUnavailable;
        return Outcome;
    }
    if (!StatsComponent->IsStatMutationAuthority())
    {
        Outcome.Result = EPassiveAllocationResult::Failed_NotAuthority;
        return Outcome;
    }

    FPassiveSkillTreeState PreparedState = State;
    Outcome = UPassiveSkillTreeFunctionLibrary::RefundNode(
        Tree, PreparedState, NodeId);
    if (Outcome.Result != EPassiveAllocationResult::Success)
    {
        return Outcome;
    }

    if (const FCharacterStatSource* Existing = StatsComponent->StatSources.Find(PassiveTreeSourceId))
    {
        if (Existing->SourceType != ECharacterStatSourceType::PassiveTree)
        {
            Outcome.Result = EPassiveAllocationResult::Failed_StatRefresh;
            Outcome.RemainingPoints = State.AvailablePoints;
            return Outcome;
        }
    }

    const FPassiveSkillTreeState OriginalState = State;
    State = MoveTemp(PreparedState);
    if (!RefreshPassiveStats(StatsComponent, Tree, State,
                             EResourceRecalculationPolicy::PreserveCurrent))
    {
        State = OriginalState;
        Outcome.Result = EPassiveAllocationResult::Failed_StatRefresh;
        Outcome.RemainingPoints = State.AvailablePoints;
        return Outcome;
    }
    return Outcome;
}
