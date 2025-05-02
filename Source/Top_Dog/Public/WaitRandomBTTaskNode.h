// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "WaitRandomBTTaskNode.generated.h"

struct FWaitRandomMemory
{
	float EndTime;
};

UCLASS()
class TOP_DOG_API UWaitRandomBTTaskNode : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UWaitRandomBTTaskNode();

    UPROPERTY(EditAnywhere, Category = "Wait")
    float MinWait = 0.2f;

    /** Tiempo máximo de espera (s) */
    UPROPERTY(EditAnywhere, Category = "Wait")
    float MaxWait = 1.5f;

    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;

    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds
    ) override;

    // Reservamos memoria para EndTime
    virtual uint16 GetInstanceMemorySize() const override
    {
        return sizeof(FWaitRandomMemory);
    }
};
