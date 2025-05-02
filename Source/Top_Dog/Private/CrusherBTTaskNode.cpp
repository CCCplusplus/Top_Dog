// Fill out your copyright notice in the Description page of Project Settings.


#include "CrusherBTTaskNode.h"

UCrusherBTTaskNode::UCrusherBTTaskNode()
{
}

EBTNodeResult::Type UCrusherBTTaskNode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UWorld* World = OwnerComp.GetWorld();
	if (!World)
		return EBTNodeResult::Failed;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AChickenMan::StaticClass(), FoundActors);

	if (FoundActors.Num() == 0)
		return EBTNodeResult::Failed;

	AActor* player = FoundActors[0];
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	BB->SetValueAsObject(TargetKey.SelectedKeyName, player);

	return EBTNodeResult::Succeeded;
}
