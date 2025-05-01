// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Ultra_Jump_Boy.generated.h"


UCLASS()
class TOP_DOG_API AUltra_Jump_Boy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AUltra_Jump_Boy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Variables  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Settings")
	float jumpHeight = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Settings")
	float JumpDistance = 200.0f;

	FVector InitialCameraLocation;

	float InitialCharacterZ;

	//skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComponent;

	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera; 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TT_Move;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TT_Jump;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> TimerWidgetClass; 

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> HeightWidgetClass;

	UPROPERTY()
	UUserWidget* TimerWidget;

	UPROPERTY()
	UUserWidget* HeightWidget;

	UPROPERTY()
	UTextBlock* TimerTextBlock;

	UPROPERTY()
	UTextBlock* HeightTextBlock;

	FTimerHandle     TimerHandle;
	int32            RemainingTime = 45;

	// Helpers UI
	void UpdateTimer();                  
	void RefreshTimerUI();               

	
	void Move(const FInputActionValue& Value);
	void JumpAction(const FInputActionValue& Value);

};
