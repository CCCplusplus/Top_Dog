// CrusherBTTaskNode.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ChickenMan.h"
#include "CrusherBTTaskNode.generated.h"

UCLASS()
class TOP_DOG_API UCrusherBTTaskNode : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCrusherBTTaskNode();

	/** Clave de Blackboard donde se guardará la referencia al ChickenMan */
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector TargetKey;

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;
};
