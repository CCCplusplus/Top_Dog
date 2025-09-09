// Fill out your copyright notice in the Description page of Project Settings.


#include "ChickenMan.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "AGMChicken_Game.h"

// Sets default values
AChickenMan::AChickenMan()
{
	PrimaryActorTick.bCanEverTick = true;

	hasturned = false;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;


	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = -300.0f;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritYaw = false;

	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 50.0f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetAbsolute(false, true, false);

	bEliminated = false;
	CurrentDistanceMeters = 0.f;
}

// Called when the game starts or when spawned
void AChickenMan::BeginPlay()
{
	Super::BeginPlay();

	if (!CrusherActor)
	{
		CrusherActor = FindNearestCrusherPawn();
		if (CrusherActor) 
		{
			UE_LOG(LogTemp, Warning, TEXT("%s linked to %s"),
				*GetName(), *CrusherActor->GetName());
		}
		else
			UE_LOG(LogTemp, Error, TEXT("%s: NO Crusher found"), *GetName());
	}

	
	activated = false;

	//set the player ID using the name of the pawn with "BPChickenMan_C_0" being "Player_1" an so forth
	const int32 LaneIndex = GetLaneIndex();
	if (LaneIndex > 0)
		PlayerID = FName(*FString::Printf(TEXT("Player_%d"), LaneIndex));
	else
		PlayerID = FName(TEXT("None"));

}

void AChickenMan::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
				Subsys->AddMappingContext(DefaultMappingContext, 0);
		}

		if (!DistanceWidget && DistanceWidgetClass)
		{
			DistanceWidget = CreateWidget<UUserWidget>(PC, DistanceWidgetClass);
			if (DistanceWidget)
			{
				DistanceWidget->AddToPlayerScreen(10);
				DistanceTextBlock = Cast<UTextBlock>(
					DistanceWidget->GetWidgetFromName(TEXT("HeightValue")));
			}
		}
	}
}

void AChickenMan::UnPossessed()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			if (DefaultMappingContext)
				Subsys->RemoveMappingContext(DefaultMappingContext);

	Super::UnPossessed();
}

// Called every frame
void AChickenMan::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsBot && !activated)
	{
		activated = true;

		const float Delay = FMath::FRandRange(12.0f, 22.0f);

		GetWorldTimerManager().ClearTimer(BotTurnHandle);

		GetWorldTimerManager().SetTimer(
			BotTurnHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (!hasturned)
						TurnAction(FInputActionValue());
				}),
			Delay,
			false
		);
	}

	if (!DistanceTextBlock || !CrusherActor || bEliminated)
		return;

	const FVector PlayerLoc = GetActorLocation();

	
	FVector ClosestPoint = CrusherActor->GetActorLocation();
	if (UPrimitiveComponent* Prim =
		CrusherActor->FindComponentByClass<UPrimitiveComponent>())
	{
		
		FVector OutPoint;
		if (Prim->GetClosestPointOnCollision(PlayerLoc, OutPoint))
			ClosestPoint = OutPoint;

		
	}

	
	float DistCm = FVector::Dist(PlayerLoc, ClosestPoint);
	float DistM = DistCm / 100.f;
	float Rounded = FMath::RoundToFloat(DistM * 100.f) / 100.f;

	
	DistanceTextBlock->SetText(FText::AsNumber(Rounded));

	CurrentDistanceMeters = DistM;
}

// Called to bind functionality to input
void AChickenMan::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (DefaultMappingContext)
			EnhancedInputComponent->BindAction(CG_Turn, ETriggerEvent::Started, this, &AChickenMan::TurnAction);
	}
}


void AChickenMan::TurnAction(const FInputActionValue& Value)
{
	if (hasturned) return;

	UWorld* W = GetWorld();
	if (!W || W->bIsTearingDown) return;

	hasturned = true;

	if (AAGMChicken_Game* GM = W->GetAuthGameMode<AAGMChicken_Game>())
		GM->NotifyPlayerFinished(this);

	FRotator RelRot = GetMesh()->GetRelativeRotation();

	const float NewYaw = FMath::IsNearlyEqual(RelRot.Yaw, -90.f, 1.f)
		? 90.f
		: -90.f;

	RelRot.Yaw = NewYaw;
	GetMesh()->SetRelativeRotation(RelRot);

	FVector Dir = GetActorForwardVector();
	LaunchCharacter(-Dir * 400.f, true, true);

	if (APawn* CrusherPawn = Cast<APawn>(CrusherActor))
	{
		if (AAIController* AICon = Cast<AAIController>(CrusherPawn->GetController()))
			if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
				BB->SetValueAsBool(TEXT("StopMoving"), true);
	}

}

void AChickenMan::Eliminate()
{
	if (bEliminated) return;
	if (bEliminated) return;
	bEliminated = true;

	GetWorldTimerManager().ClearTimer(BotTurnHandle);

	CurrentDistanceMeters = FLT_MAX;

	if (DistanceTextBlock)
		DistanceTextBlock->SetText(FText::FromString(TEXT("XX.XX")));


	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector CamLoc; FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ACameraActor* StaticCam =
			GetWorld()->SpawnActor<ACameraActor>(CamLoc, CamRot, Params);

		if (StaticCam && PC->PlayerCameraManager)
			StaticCam->GetCameraComponent()->FieldOfView =
			PC->PlayerCameraManager->GetFOVAngle();

		PC->SetViewTargetWithBlend(StaticCam, 0.f);
	}


	{
		FVector CamLoc; FRotator CamRot;
		if (APlayerController* PC0 = UGameplayStatics::GetPlayerController(this, 0))
			PC0->GetPlayerViewPoint(CamLoc, CamRot);

		const FVector DirToCam = (CamLoc - GetActorLocation()).GetSafeNormal();
		const FVector LaunchVel = DirToCam * 2000.f + FVector(0.f, 0.f, 400.f);
		LaunchCharacter(LaunchVel, true, true);
	}


	{
		FTimerHandle Tmp;
		GetWorldTimerManager().SetTimer(
			Tmp,
			[this]()
			{
				SetActorEnableCollision(false);
				SetActorHiddenInGame(true);

				if (AAGMChicken_Game* GM = GetWorld()->GetAuthGameMode<AAGMChicken_Game>())
					GM->OnPlayerEliminated(this);
			},
			0.5f,
			false
		);
	}
}

void AChickenMan::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BotTurnHandle);

	if (DistanceWidget)
	{
		DistanceWidget->RemoveFromParent();
		DistanceWidget = nullptr;
		DistanceTextBlock = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

APawn* AChickenMan::FindNearestCrusherPawn() const
{
	const int32 Lane = GetLaneIndex();
	if (Lane == 0) return nullptr;

	const FName WantedTag(*FString::Printf(TEXT("Crusher_%d"), Lane));

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), WantedTag, Found);

	for (AActor* A : Found)
		if (APawn* P = Cast<APawn>(A))
			return P;

	return nullptr;
}


int32 AChickenMan::GetLaneIndex() const
{
	const FString Name = GetName();
	const int32   Pos = Name.Find(TEXT("_C_"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	if (Pos != INDEX_NONE)
	{
		const int32 X = FCString::Atoi(*Name.Mid(Pos + 3));
		const int32 N = X + 1;                             
		if (N >= 1 && N <= 4) return N;
	}
	return 0;
}
