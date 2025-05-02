// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "RandomAdvanceBTTaskNode.generated.h"

/**
 * 
 */
UCLASS()
class TOP_DOG_API URandomAdvanceBTTaskNode : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
    URandomAdvanceBTTaskNode();

    /** Distancia mínima a avanzar */
    UPROPERTY(EditAnywhere, Category = "Advance")
    float MinDistance = 500.f;

    /** Distancia máxima a avanzar */
    UPROPERTY(EditAnywhere, Category = "Advance")
    float MaxDistance = 1000.f;

    /** Blackboard key que contiene el actor jugador */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector PlayerKey;

    /** Blackboard key donde volcamos el Vector destino */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetPosKey;

    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;
};
