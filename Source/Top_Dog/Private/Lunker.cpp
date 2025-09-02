#include "Lunker.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

ALunker::ALunker()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = -300.f;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritYaw = false;
	SpringArm->SocketOffset = FVector(0, 0, 50);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	UCapsuleComponent* Cap = GetCapsuleComponent();
	Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Cap->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->BrakingDecelerationWalking = 2000.f;
	Move->AirControl = 0.5f;
	Move->JumpZVelocity = 600.f;
}

void ALunker::BeginPlay()
{
	Super::BeginPlay();

	const int32 Idx = GetIndexFromName();
	if (Idx > 0)
	{
		const FName Tag(*FString::Printf(TEXT("Player_%d"), Idx));
		if (!Tags.Contains(Tag)) Tags.Add(Tag);
	}
}

void ALunker::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext) Subsys->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ALunker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsBot && !bEliminated && bBotMoving)
	{
		const FVector Me = GetActorLocation();
		FVector To = BotTarget - Me; To.Z = 0.f;
		const float Len = To.Size();
		if (Len > KINDA_SMALL_NUMBER) { AddMovementInput(To / Len, 1.f); }
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now >= BotNextJumpTime) { Jump(); BotNextJumpTime = Now + FMath::FRandRange(0.6f, 1.2f); }
	}

	const FVector Vel2D = GetVelocity();
	FVector VelFlat = Vel2D; VelFlat.Z = 0.f;
	if (VelFlat.SizeSquared() > 25.f)
	{
		const float TargetYaw = VelFlat.ToOrientationRotator().Yaw;
		const FRotator NewRot(0.f, TargetYaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRot, DeltaTime, 10.f));
	}
}


void ALunker::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Move)  EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ALunker::Move);
		if (IA_Jump)  EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ALunker::JumpAction);
	}
}

void ALunker::Move(const FInputActionValue& Value)
{
	if (bEliminated || bControlsLocked) return;

	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero()) return;

	FRotator CamRot = GetActorRotation();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector ViewLoc;
		PC->GetPlayerViewPoint(ViewLoc, CamRot);
	}
	const FRotator YawOnly(0.f, CamRot.Yaw, 0.f);


	const FVector Fwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

	FVector Dir = (Fwd * Axis.Y) + (Right * Axis.X);
	Dir.Z = 0.f;

	if (!Dir.IsNearlyZero())
	{
		Dir.Normalize();
		AddMovementInput(Dir, 1.f);

		const float TargetYaw = Dir.ToOrientationRotator().Yaw;
		SetActorRotation(FRotator(0.f, TargetYaw, 0.f));
	}
}



void ALunker::JumpAction(const FInputActionValue& Value)
{
	if (bEliminated || bControlsLocked) return;
	
	Jump();
}


void ALunker::OnNewRound(const FVector& InTarget, float DelaySeconds)
{
	if (bEliminated) return;
	BotTarget = InTarget;
	GetWorldTimerManager().ClearTimer(BotDelayHandle);
	GetWorldTimerManager().SetTimer(BotDelayHandle, [this]()
		{
			bBotMoving = true;
			BotNextJumpTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(0.0f, 0.4f);
			Jump();
		}, FMath::Max(0.f, DelaySeconds), false);
}

void ALunker::Eliminate()
{
	if (bEliminated) return;
	bEliminated = true;
	bBotMoving = false;

	GetWorldTimerManager().ClearTimer(BotDelayHandle);

	if (AController* C = GetController())
	{
		if (APlayerController* PC = Cast<APlayerController>(C))
			DisableInput(PC);
		C->UnPossess();
	}

	if (UCapsuleComponent* Cap = GetCapsuleComponent())
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LaunchCharacter(FVector(0, 0, -600.f), true, false);

	SetLifeSpan(3.0f);
}

void ALunker::LockInputs()
{
	if (bControlsLocked || bEliminated) return;
	bControlsLocked = true;

	bBotMoving = false;
	StopJumping();

	if (AController* C = GetController())
		if (APlayerController* PC = Cast<APlayerController>(C))
			DisableInput(PC);
}

bool ALunker::HasCollisionDisabled() const
{
	if (const UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		return Cap->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
	}
	return true;
}


int32 ALunker::GetIndexFromName() const
{
	const FString N = GetName();
	const int32 P = N.Find(TEXT("_C_"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (P != INDEX_NONE)
	{
		const int32 X = FCString::Atoi(*N.Mid(P + 3));
		const int32 N1 = X + 1;
		if (N1 >= 1 && N1 <= 4) return N1;
	}
	return 0;
}
