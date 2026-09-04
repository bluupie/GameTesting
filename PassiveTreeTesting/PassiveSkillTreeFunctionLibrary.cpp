#include "PassiveSkillTreeFunctionLibrary.h"
#include "Containers/Queue.h"

const FPassiveSkillNode* UPassiveSkillTreeFunctionLibrary::FindNode(const FPassiveSkillTreeDefinition& Tree, FName NodeId)
{
    return Tree.Nodes.FindByPredicate([NodeId](const FPassiveSkillNode& Node) { return Node.NodeId == NodeId; });
}

bool UPassiveSkillTreeFunctionLibrary::AreConnected(const FPassiveSkillTreeDefinition& Tree, FName A, FName B)
{
    const FPassiveSkillNode* NodeA = FindNode(Tree, A);
    const FPassiveSkillNode* NodeB = FindNode(Tree, B);
    return NodeA && NodeB && (NodeA->ConnectedNodeIds.Contains(B) || NodeB->ConnectedNodeIds.Contains(A));
}

FPassiveSkillTreeState UPassiveSkillTreeFunctionLibrary::CreateInitialState(EPassiveAttributeSection StartingSection,
                                                                             int32 AvailablePoints)
{
    FPassiveSkillTreeState State;
    State.AvailablePoints = FMath::Max(0, AvailablePoints);
    switch (StartingSection)
    {
        case EPassiveAttributeSection::Strength:     State.ActiveStartNodeIds.Add(FName("Start_Strength")); break;
        case EPassiveAttributeSection::Intelligence: State.ActiveStartNodeIds.Add(FName("Start_Intelligence")); break;
        case EPassiveAttributeSection::Dexterity:    State.ActiveStartNodeIds.Add(FName("Start_Dexterity")); break;
    }
    return State;
}

EPassiveAllocationResult UPassiveSkillTreeFunctionLibrary::CanAllocateNode(const FPassiveSkillTreeDefinition& Tree,
                                                                             const FPassiveSkillTreeState& State,
                                                                             FName NodeId)
{
    const FPassiveSkillNode* Node = FindNode(Tree, NodeId);
    if (!Node) return EPassiveAllocationResult::Failed_NodeNotFound;
    if (State.AllocatedNodeIds.Contains(NodeId)) return EPassiveAllocationResult::Failed_AlreadyAllocated;
    if (State.AvailablePoints < Node->PointCost) return EPassiveAllocationResult::Failed_NoPoints;

    if (Node->NodeType == EPassiveNodeType::Start)
    {
        return State.ActiveStartNodeIds.Contains(NodeId)
            ? EPassiveAllocationResult::Success
            : EPassiveAllocationResult::Failed_StartNodeLocked;
    }

    for (FName AllocatedId : State.AllocatedNodeIds)
    {
        if (AreConnected(Tree, NodeId, AllocatedId)) return EPassiveAllocationResult::Success;
    }
    return EPassiveAllocationResult::Failed_NotConnected;
}

FPassiveAllocationOutcome UPassiveSkillTreeFunctionLibrary::AllocateNode(const FPassiveSkillTreeDefinition& Tree,
                                                                           FPassiveSkillTreeState& State, FName NodeId)
{
    FPassiveAllocationOutcome Outcome;
    Outcome.Result = CanAllocateNode(Tree, State, NodeId);
    if (Outcome.Result == EPassiveAllocationResult::Success)
    {
        const FPassiveSkillNode* Node = FindNode(Tree, NodeId);
        State.AvailablePoints -= Node->PointCost;
        State.AllocatedNodeIds.Add(NodeId);
    }
    Outcome.RemainingPoints = State.AvailablePoints;
    return Outcome;
}

bool UPassiveSkillTreeFunctionLibrary::AllAllocatedNodesReachAStart(const FPassiveSkillTreeDefinition& Tree,
                                                                    const FPassiveSkillTreeState& State,
                                                                    const TSet<FName>& AllocationSet)
{
    if (AllocationSet.Num() == 0) return true;

    TSet<FName> Visited;
    TQueue<FName> Pending;
    for (FName StartId : State.ActiveStartNodeIds)
    {
        if (AllocationSet.Contains(StartId))
        {
            Visited.Add(StartId);
            Pending.Enqueue(StartId);
        }
    }

    FName Current;
    while (Pending.Dequeue(Current))
    {
        for (FName Candidate : AllocationSet)
        {
            if (!Visited.Contains(Candidate) && AreConnected(Tree, Current, Candidate))
            {
                Visited.Add(Candidate);
                Pending.Enqueue(Candidate);
            }
        }
    }
    return Visited.Num() == AllocationSet.Num();
}

EPassiveAllocationResult UPassiveSkillTreeFunctionLibrary::CanRefundNode(const FPassiveSkillTreeDefinition& Tree,
                                                                           const FPassiveSkillTreeState& State,
                                                                           FName NodeId)
{
    const FPassiveSkillNode* Node = FindNode(Tree, NodeId);
    if (!Node) return EPassiveAllocationResult::Failed_NodeNotFound;
    if (!State.AllocatedNodeIds.Contains(NodeId)) return EPassiveAllocationResult::Failed_NotAllocated;
    if (Node->NodeType == EPassiveNodeType::Start) return EPassiveAllocationResult::Failed_StartNodeCannotBeRefunded;

    TSet<FName> Remaining = State.AllocatedNodeIds;
    Remaining.Remove(NodeId);
    return AllAllocatedNodesReachAStart(Tree, State, Remaining)
        ? EPassiveAllocationResult::Success
        : EPassiveAllocationResult::Failed_WouldDisconnectTree;
}

FPassiveAllocationOutcome UPassiveSkillTreeFunctionLibrary::RefundNode(const FPassiveSkillTreeDefinition& Tree,
                                                                         FPassiveSkillTreeState& State, FName NodeId)
{
    FPassiveAllocationOutcome Outcome;
    Outcome.Result = CanRefundNode(Tree, State, NodeId);
    if (Outcome.Result == EPassiveAllocationResult::Success)
    {
        const FPassiveSkillNode* Node = FindNode(Tree, NodeId);
        State.AllocatedNodeIds.Remove(NodeId);
        State.AvailablePoints += Node->PointCost;
    }
    Outcome.RemainingPoints = State.AvailablePoints;
    return Outcome;
}

TArray<FName> UPassiveSkillTreeFunctionLibrary::GetAllocatableNodes(const FPassiveSkillTreeDefinition& Tree,
                                                                     const FPassiveSkillTreeState& State)
{
    TArray<FName> Result;
    for (const FPassiveSkillNode& Node : Tree.Nodes)
    {
        if (CanAllocateNode(Tree, State, Node.NodeId) == EPassiveAllocationResult::Success) Result.Add(Node.NodeId);
    }
    return Result;
}

FPassiveStatContribution UPassiveSkillTreeFunctionLibrary::BuildPassiveContribution(const FPassiveSkillTreeDefinition& Tree,
                                                                                      const FPassiveSkillTreeState& State)
{
    FPassiveStatContribution Result;
    auto ApplyStat = [&Result](EStatType Type, EPassiveModifierApplication Application, float Value)
    {
        switch (Application)
        {
            case EPassiveModifierApplication::Flat:      Result.StatPool.AddFlat(Type, Value); break;
            case EPassiveModifierApplication::Increased: Result.StatPool.AddIncreased(Type, Value); break;
            case EPassiveModifierApplication::More:      Result.StatPool.AddMore(Type, Value); break;
        }
    };

    for (const FPassiveSkillNode& Node : Tree.Nodes)
    {
        if (!State.AllocatedNodeIds.Contains(Node.NodeId)) continue;
        for (const FPassiveNodeEffect& Effect : Node.Effects)
        {
            if (Effect.Target == EPassiveEffectTarget::Attribute)
            {
                const int32 AttributeValue = FMath::RoundToInt(Effect.Value);
                switch (Effect.AttributeType)
                {
                    case EPassiveAttributeType::Strength:     Result.BonusStrength += AttributeValue; break;
                    case EPassiveAttributeType::Dexterity:    Result.BonusDexterity += AttributeValue; break;
                    case EPassiveAttributeType::Intelligence: Result.BonusIntelligence += AttributeValue; break;
                }
                continue;
            }

            ApplyStat(Effect.StatType, Effect.Application, Effect.Value);
            for (EStatType AdditionalType : Effect.AdditionalStatTypes)
            {
                ApplyStat(AdditionalType, Effect.Application, Effect.Value);
            }
        }
    }
    return Result;
}

FBaseAttributes UPassiveSkillTreeFunctionLibrary::ApplyPassiveAttributes(const FBaseAttributes& BaseAttributes,
                                                                          const FPassiveStatContribution& Contribution)
{
    FBaseAttributes Result = BaseAttributes;
    Result.Strength += Contribution.BonusStrength;
    Result.Dexterity += Contribution.BonusDexterity;
    Result.Intelligence += Contribution.BonusIntelligence;
    return Result;
}

void UPassiveSkillTreeFunctionLibrary::MergeModifierPool(FModifierPool& Target, const FModifierPool& Source)
{
    for (const TPair<EStatType, FStatModifier>& Pair : Source.Mods)
    {
        FStatModifier& TargetMod = Target.Mods.FindOrAdd(Pair.Key);
        TargetMod.Flat += Pair.Value.Flat;
        TargetMod.Percent += Pair.Value.Percent;
        TargetMod.MorePercent *= Pair.Value.MorePercent;
    }
}

bool UPassiveSkillTreeFunctionLibrary::ValidateTree(const FPassiveSkillTreeDefinition& Tree, TArray<FString>& OutErrors)
{
    OutErrors.Reset();
    TSet<FName> KnownIds;
    for (const FPassiveSkillNode& Node : Tree.Nodes)
    {
        if (Node.NodeId.IsNone()) OutErrors.Add(TEXT("A passive node has an empty NodeId."));
        else if (KnownIds.Contains(Node.NodeId)) OutErrors.Add(FString::Printf(TEXT("Duplicate NodeId: %s"), *Node.NodeId.ToString()));
        KnownIds.Add(Node.NodeId);
        if (Node.PointCost < 0) OutErrors.Add(FString::Printf(TEXT("Node %s has a negative point cost."), *Node.NodeId.ToString()));
    }
    for (const FPassiveSkillNode& Node : Tree.Nodes)
    {
        for (FName ConnectedId : Node.ConnectedNodeIds)
        {
            if (!KnownIds.Contains(ConnectedId))
                OutErrors.Add(FString::Printf(TEXT("Node %s references missing node %s."), *Node.NodeId.ToString(), *ConnectedId.ToString()));
            if (ConnectedId == Node.NodeId)
                OutErrors.Add(FString::Printf(TEXT("Node %s connects to itself."), *Node.NodeId.ToString()));
        }
    }
    return OutErrors.Num() == 0;
}

FPassiveSkillTreeDefinition UPassiveSkillTreeFunctionLibrary::GetExampleThreeSectionTree()
{
    FPassiveSkillTreeDefinition Tree;

    auto Attribute = [](EPassiveAttributeType Type, float Value)
    {
        FPassiveNodeEffect E; E.Target = EPassiveEffectTarget::Attribute; E.AttributeType = Type; E.Value = Value; return E;
    };
    auto Stat = [](EStatType Type, EPassiveModifierApplication Application, float Value,
                   TArray<EStatType> Additional = TArray<EStatType>())
    {
        FPassiveNodeEffect E; E.Target = EPassiveEffectTarget::Stat; E.StatType = Type;
        E.Application = Application; E.Value = Value; E.AdditionalStatTypes = MoveTemp(Additional); return E;
    };
    auto Add = [&Tree](const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
                       EPassiveAttributeSection Section, EPassiveNodeType Type, FVector2D Position,
                       TArray<FName> Connections, TArray<FPassiveNodeEffect> Effects, int32 Cost = 1)
    {
        FPassiveSkillNode Node; Node.NodeId = FName(Id); Node.DisplayName = Name; Node.Description = Description;
        Node.Section = Section; Node.NodeType = Type; Node.PointCost = Cost; Node.TreePosition = Position;
        Node.ConnectedNodeIds = MoveTemp(Connections); Node.Effects = MoveTemp(Effects); Tree.Nodes.Add(MoveTemp(Node));
    };

    // Strength: durable melee, physical damage and weapon specialization.
    Add(TEXT("Start_Strength"), TEXT("Strength Start"), TEXT("The Strength starting point."), EPassiveAttributeSection::Strength,
        EPassiveNodeType::Start, {-700, 0}, {FName("Str_Strength1")}, {Attribute(EPassiveAttributeType::Strength, 10)}, 0);
    Add(TEXT("Str_Strength1"), TEXT("Might"), TEXT("+10 Strength"), EPassiveAttributeSection::Strength,
        EPassiveNodeType::Minor, {-580, 0}, {FName("Str_Physical1")}, {Attribute(EPassiveAttributeType::Strength, 10)});
    Add(TEXT("Str_Physical1"), TEXT("Brutality"), TEXT("12% increased Physical Damage"), EPassiveAttributeSection::Strength,
        EPassiveNodeType::Minor, {-460, 0}, {FName("Str_AttackSpeed"), FName("Str_Swords"), FName("Str_Axes"), FName("Dex_Physical1")},
        {Stat(EStatType::DamagePhysical, EPassiveModifierApplication::Increased, 0.12f)});
    Add(TEXT("Str_AttackSpeed"), TEXT("Relentless Assault"), TEXT("8% increased Attack Speed"), EPassiveAttributeSection::Strength,
        EPassiveNodeType::Minor, {-340, -120}, {FName("Str_Warrior")}, {Stat(EStatType::AttackSpeed, EPassiveModifierApplication::Increased, 0.08f)});
    Add(TEXT("Str_Swords"), TEXT("Blade Mastery"), TEXT("25% increased Damage with Swords"), EPassiveAttributeSection::Strength,
        EPassiveNodeType::Notable, {-340, 0}, {FName("Str_Warrior")}, {Stat(EStatType::DamageWithSwords, EPassiveModifierApplication::Increased, 0.25f)});
    Add(TEXT("Str_Axes"), TEXT("Axe Mastery"), TEXT("25% increased Damage with Axes"), EPassiveAttributeSection::Strength,
        EPassiveNodeType::Notable, {-340, 120}, {FName("Str_Warrior")}, {Stat(EStatType::DamageWithAxes, EPassiveModifierApplication::Increased, 0.25f)});
    Add(TEXT("Str_Warrior"), TEXT("Juggernaut"), TEXT("More melee damage and maximum life."), EPassiveAttributeSection::Strength,
        EPassiveNodeType::Keystone, {-180, 0}, {}, {Stat(EStatType::DamagePhysical, EPassiveModifierApplication::More, 0.12f),
        Stat(EStatType::Life, EPassiveModifierApplication::Increased, 0.15f)});

    // Intelligence: spell casting, mana and the three elemental damage types.
    Add(TEXT("Start_Intelligence"), TEXT("Intelligence Start"), TEXT("The Intelligence starting point."), EPassiveAttributeSection::Intelligence,
        EPassiveNodeType::Start, {350, -600}, {FName("Int_Intelligence1")}, {Attribute(EPassiveAttributeType::Intelligence, 10)}, 0);
    Add(TEXT("Int_Intelligence1"), TEXT("Insight"), TEXT("+10 Intelligence"), EPassiveAttributeSection::Intelligence,
        EPassiveNodeType::Minor, {290, -490}, {FName("Int_SpellDamage")}, {Attribute(EPassiveAttributeType::Intelligence, 10)});
    Add(TEXT("Int_SpellDamage"), TEXT("Arcane Power"), TEXT("12% increased Spell Damage"), EPassiveAttributeSection::Intelligence,
        EPassiveNodeType::Minor, {230, -380}, {FName("Int_CastSpeed"), FName("Int_Elemental")},
        {Stat(EStatType::SpellDamage, EPassiveModifierApplication::Increased, 0.12f)});
    Add(TEXT("Int_CastSpeed"), TEXT("Quickened Thought"), TEXT("8% increased Cast Speed"), EPassiveAttributeSection::Intelligence,
        EPassiveNodeType::Minor, {100, -310}, {FName("Int_Archmage")}, {Stat(EStatType::CastSpeed, EPassiveModifierApplication::Increased, 0.08f)});
    Add(TEXT("Int_Elemental"), TEXT("Elementalist"), TEXT("15% increased Elemental Damage"), EPassiveAttributeSection::Intelligence,
        EPassiveNodeType::Notable, {350, -270}, {FName("Int_Archmage"), FName("Dex_Elemental")},
        {Stat(EStatType::DamageFire, EPassiveModifierApplication::Increased, 0.15f,
        {EStatType::DamageCold, EStatType::DamageLightning})});
    Add(TEXT("Int_Archmage"), TEXT("Archmage"), TEXT("More Spell Damage and increased maximum Mana."), EPassiveAttributeSection::Intelligence,
        EPassiveNodeType::Keystone, {220, -150}, {}, {Stat(EStatType::SpellDamage, EPassiveModifierApplication::More, 0.15f),
        Stat(EStatType::Mana, EPassiveModifierApplication::Increased, 0.20f)});

    // Dexterity: speed, projectiles and hybrid physical/elemental offense.
    Add(TEXT("Start_Dexterity"), TEXT("Dexterity Start"), TEXT("The Dexterity starting point."), EPassiveAttributeSection::Dexterity,
        EPassiveNodeType::Start, {350, 600}, {FName("Dex_Dexterity1")}, {Attribute(EPassiveAttributeType::Dexterity, 10)}, 0);
    Add(TEXT("Dex_Dexterity1"), TEXT("Agility"), TEXT("+10 Dexterity"), EPassiveAttributeSection::Dexterity,
        EPassiveNodeType::Minor, {290, 490}, {FName("Dex_AttackSpeed")}, {Attribute(EPassiveAttributeType::Dexterity, 10)});
    Add(TEXT("Dex_AttackSpeed"), TEXT("Finesse"), TEXT("10% increased Attack Speed"), EPassiveAttributeSection::Dexterity,
        EPassiveNodeType::Minor, {230, 380}, {FName("Dex_Physical1"), FName("Dex_Elemental")},
        {Stat(EStatType::AttackSpeed, EPassiveModifierApplication::Increased, 0.10f)});
    Add(TEXT("Dex_Physical1"), TEXT("Precise Strikes"), TEXT("12% increased Physical Damage"), EPassiveAttributeSection::Dexterity,
        EPassiveNodeType::Minor, {100, 310}, {FName("Dex_Projectiles"), FName("Str_Physical1")},
        {Stat(EStatType::DamagePhysical, EPassiveModifierApplication::Increased, 0.12f)});
    Add(TEXT("Dex_Elemental"), TEXT("Primal Shots"), TEXT("12% increased Elemental Damage"), EPassiveAttributeSection::Dexterity,
        EPassiveNodeType::Minor, {350, 270}, {FName("Dex_Projectiles"), FName("Int_Elemental")},
        {Stat(EStatType::DamageFire, EPassiveModifierApplication::Increased, 0.12f,
        {EStatType::DamageCold, EStatType::DamageLightning})});
    Add(TEXT("Dex_Projectiles"), TEXT("Projectile Mastery"), TEXT("20% increased Projectile Damage"), EPassiveAttributeSection::Dexterity,
        EPassiveNodeType::Notable, {220, 150}, {FName("Dex_Multishot")},
        {Stat(EStatType::ProjectileDamage, EPassiveModifierApplication::Increased, 0.20f)});
    Add(TEXT("Dex_Multishot"), TEXT("Multishot"), TEXT("Skills fire 1 additional Projectile."), EPassiveAttributeSection::Dexterity,
        EPassiveNodeType::Keystone, {100, 40}, {}, {Stat(EStatType::AdditionalProjectiles, EPassiveModifierApplication::Flat, 1.0f)});

    return Tree;
}
