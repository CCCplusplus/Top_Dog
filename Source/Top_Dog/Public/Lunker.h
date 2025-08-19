#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Lunker.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class TOP_DOG_API ALunker : public ACharacter
{
	GENERATED_BODY()

public:
	ALunker();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Move;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Jump;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Bot")
	bool bIsBot = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "State")
	bool bEliminated = false;

	UFUNCTION(BlueprintCallable, Category = "Round")
	void OnNewRound(const FVector& InTarget, float DelaySeconds);

	UFUNCTION(BlueprintCallable, Category = "Round")
	void Eliminate();

	bool bControlsLocked = false;

	UFUNCTION(BlueprintCallable) void LockInputs();
	UFUNCTION(BlueprintCallable) bool AreInputsLocked() const { return bControlsLocked; }
	UFUNCTION(BlueprintCallable) bool HasCollisionDisabled() const;


protected:
	void Move(const FInputActionValue& Value);
	void JumpAction(const FInputActionValue& Value);

	int32 GetIndexFromName() const;

	FVector BotTarget = FVector::ZeroVector;
	bool    bBotMoving = false;
	float   BotNextJumpTime = 0.f;

	FTimerHandle BotDelayHandle;
};
