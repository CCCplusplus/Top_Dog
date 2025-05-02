// Fill out your copyright notice in the Description page of Project Settings.


#include "WaitRandomBTTaskNode.h"

UWaitRandomBTTaskNode::UWaitRandomBTTaskNode()
{
	bNotifyTick = true;
}

EBTNodeResult::Type UWaitRandomBTTaskNode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    FWaitRandomMemory* Mem = (FWaitRandomMemory*)NodeMemory;
    float WaitTime = FMath::FRandRange(MinWait, MaxWait);
    Mem->EndTime = OwnerComp.GetWorld()->GetTimeSeconds() + WaitTime;

    return EBTNodeResult::InProgress;
}

void UWaitRandomBTTaskNode::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    FWaitRandomMemory* Mem = (FWaitRandomMemory*)NodeMemory;
    if (OwnerComp.GetWorld()->GetTimeSeconds() >= Mem->EndTime)
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    
}
