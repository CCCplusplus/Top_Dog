// Fill out your copyright notice in the Description page of Project Settings.


#include "CrusherBTTaskNode.h"
#include "AIController.h"
#include "EngineUtils.h"

UCrusherBTTaskNode::UCrusherBTTaskNode()
{
    bCreateNodeInstance = true;
}

EBTNodeResult::Type UCrusherBTTaskNode::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UWorld* World = OwnerComp.GetWorld();
    if (!World) return EBTNodeResult::Failed;

    APawn* SelfPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
    if (!SelfPawn || SelfPawn->Tags.Num() == 0) return EBTNodeResult::Failed;

    FString MyTag = SelfPawn->Tags[0].ToString();
    MyTag.ReplaceInline(TEXT("Crusher"), TEXT("Player"));

    for (TActorIterator<APawn> It(World, APawn::StaticClass()); It; ++It)
    {
        if (*It != SelfPawn && It->ActorHasTag(FName(*MyTag)))
        {
            if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
            {
                BB->SetValueAsObject(TargetKey.SelectedKeyName, *It);
            }
            return EBTNodeResult::Succeeded;
        }
    }

    return EBTNodeResult::Failed;
}

