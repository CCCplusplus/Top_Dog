// Fill out your copyright notice in the Description page of Project Settings.


#include "RandomAdvanceBTTaskNode.h"

URandomAdvanceBTTaskNode::URandomAdvanceBTTaskNode()
{
}

EBTNodeResult::Type URandomAdvanceBTTaskNode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    auto* BB = OwnerComp.GetBlackboardComponent();
    UObject* Obj = BB->GetValueAsObject(PlayerKey.SelectedKeyName);
    APawn* PlayerPawn = Cast<APawn>(Obj);
    APawn* SelfPawn = OwnerComp.GetAIOwner()->GetPawn<APawn>();

    if (!PlayerPawn || !SelfPawn)
        return EBTNodeResult::Failed;

    FVector ToPlayer = (PlayerPawn->GetActorLocation() - SelfPawn->GetActorLocation()).GetSafeNormal();
    float  Dist = FMath::FRandRange(MinDistance, MaxDistance);

    FVector Dest = SelfPawn->GetActorLocation() + ToPlayer * Dist;
    BB->SetValueAsVector(TargetPosKey.SelectedKeyName, Dest);

    return EBTNodeResult::Succeeded;
}
