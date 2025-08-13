// Fill out your copyright notice in the Description page of Project Settings.


#include "Ultra_Jump_Boy.h"
#include <Kismet/GameplayStatics.h>
#include "Blueprint/WidgetTree.h"

// Sets default values
AUltra_Jump_Boy::AUltra_Jump_Boy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a SpringArm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = -300.0f; 
	SpringArm->bUsePawnControlRotation = true; 
	
	 
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 50.0f); 

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName); 
	Camera->bUsePawnControlRotation = false; 


}

void AUltra_Jump_Boy::BeginPlay()
{
	Super::BeginPlay();

	SpringArm->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	/*InitialCameraLocation = SpringArm->GetComponentLocation();
	InitialCharacterZ = GetActorLocation().Z;*/

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->JumpZVelocity = jumpHeight;
		MoveComp->AirControl = 1.0f;
		MoveComp->AirControlBoostMultiplier = 1.0f;
		MoveComp->AirControlBoostVelocityThreshold = 0.0f;
		MoveComp->BrakingFrictionFactor = 0.0f;
	}
}

void AUltra_Jump_Boy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		if (!HeightWidget && HeightWidgetClass)
		{
			HeightWidget = CreateWidget<UUserWidget>(PC, HeightWidgetClass);
			if (HeightWidget)
			{
				HeightWidget->AddToPlayerScreen(10);
				if (UTextBlock* HB = Cast<UTextBlock>(HeightWidget->WidgetTree->FindWidget(TEXT("HeightValue"))))
				{
					HeightTextBlock = HB;
					HeightTextBlock->SetText(FText::FromString(TEXT("0.00 M")));
				}
			}
		}
	}

	if (bIsBot && !bBotActivated)
		BotActivate();
}




void AUltra_Jump_Boy::BotActivate()
{
	bBotActivated = true;
	GetWorldTimerManager().SetTimer(
		BotJumpHandle, this, &AUltra_Jump_Boy::BotDoJump, FMath::FRandRange(0.8f, 1.4f), false);
}

void AUltra_Jump_Boy::BotDoJump()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (MoveComp && !MoveComp->IsFalling())
	{
		BotMoveDir = (FMath::FRand() < 0.5f) ? -1 : +1;
		bApplyBotAirStrafe = true;   // activamos strafe para el ascenso de este salto
		Jump();
	}

	GetWorldTimerManager().SetTimer(
		BotJumpHandle, this, &AUltra_Jump_Boy::BotDoJump, FMath::FRandRange(0.9f, 1.6f), false);
}

// Called every frame
void AUltra_Jump_Boy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (!bBaseSet && MoveComp && MoveComp->IsMovingOnGround())
	{
		InitialCharacterZ = GetActorLocation().Z;
		InitialCameraLocation = SpringArm->GetComponentLocation();
		bBaseSet = true;
		if (HeightTextBlock)
			HeightTextBlock->SetText(FText::FromString(TEXT("0.00 M")));
	}

	if (!bBaseSet) return;

	const float CurrentZ = GetActorLocation().Z;
	const float DeltaZ = CurrentZ - InitialCharacterZ;

	SpringArm->SetWorldLocation(InitialCameraLocation + FVector(0.f, 0.f, DeltaZ));

	if (HeightTextBlock)
	{
		const float Meters = FMath::Max(0.f, DeltaZ * 0.01f);
		HeightTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%.2f M"), Meters)));
	}

	if (bIsBot && !bBotActivated && Controller)           // respaldo por si PossessedBy no lo activó
		BotActivate();

	if (bIsBot && MoveComp)
	{
		if (MoveComp->IsFalling())
		{
			if (MoveComp->Velocity.Z > 0.f && bApplyBotAirStrafe)   // solo en ascenso
				AddMovementInput(GetActorRightVector(), (float)BotMoveDir);
		}
		else
		{
			bApplyBotAirStrafe = false; // aterrizó
		}
	}
}



// Called to bind functionality to input
void AUltra_Jump_Boy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		
		EIC->BindAction(TT_Move, ETriggerEvent::Triggered, this, &AUltra_Jump_Boy::Move);

		
		EIC->BindAction(TT_Jump, ETriggerEvent::Started, this, &AUltra_Jump_Boy::JumpAction);
	}
}

void AUltra_Jump_Boy::Move(const FInputActionValue& Value)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp->IsFalling() && MoveComp->Velocity.Z < 0.0f)
		return;
	

	FVector2D MovementInput = Value.Get<FVector2D>();
	
	if (FMath::Abs(MovementInput.X) > KINDA_SMALL_NUMBER)
		AddMovementInput(GetActorRightVector(), -MovementInput.X);
	
	
}

void AUltra_Jump_Boy::JumpAction(const FInputActionValue& Value)
{
	Jump();
}
