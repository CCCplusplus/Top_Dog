// Fill out your copyright notice in the Description page of Project Settings.


#include "ChickenMan.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "AGMChicken_Game.h"

// Sets default values
AChickenMan::AChickenMan()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	hasturned = false;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;


	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = -300.0f;
	SpringArm->bUsePawnControlRotation = false;
	//the spring Arm should not rotate with the camera
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

	
	if (bIsBot)                      
	{
		const float Delay = FMath::FRandRange(5.f, 10.f);
		FTimerHandle Dummy;
		GetWorldTimerManager().SetTimer(Dummy, [this]()
			{
				if (!hasturned)          
					TurnAction(FInputActionValue());
			},
			Delay, false);
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		// add Enhanced Input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			Subsys->AddMappingContext(DefaultMappingContext, 0);
		
	}

	if (DistanceWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DistanceWidget = CreateWidget<UUserWidget>(PC, DistanceWidgetClass);
			if (DistanceWidget)
			{

				DistanceWidget->AddToPlayerScreen(0);
				DistanceTextBlock = Cast<UTextBlock>(
					DistanceWidget->WidgetTree->FindWidget(TEXT("HeightValue")));
			}
		}
	}
}

// Called every frame
void AChickenMan::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	CheckDistance(DistM);
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
	if (hasturned)
		return;

	FRotator RelRot = GetMesh()->GetRelativeRotation();

	const float NewYaw = FMath::IsNearlyEqual(RelRot.Yaw, -90.f, 1.f)
		? 90.f
		: -90.f;

	RelRot.Yaw = NewYaw;
	GetMesh()->SetRelativeRotation(RelRot);

	FVector Dir = GetActorForwardVector();
	LaunchCharacter(-Dir * 400.f, true, true);

	hasturned = true;

	if (!CrusherActor) 
	{
		UE_LOG(LogTemp, Error, TEXT("%s: No Crusher assigned"), *GetName());
		return;
	}

	/*if (APawn* CrusherPawn = Cast<APawn>(CrusherActor))
	{
		if (AAIController* AICon = Cast<AAIController>(CrusherPawn->GetController()))
			if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
				BB->SetValueAsBool(TEXT("StopMoving"), true);
	}*/

	CrusherActor->Destroy();


	//FTimerHandle DelayHandle;
	//GetWorldTimerManager().SetTimer(DelayHandle, [this]()
	//	{
	//		UGameplayStatics::OpenLevel(this, FName("ThirdPersonMap"));
	//	}, 2.0f, false);

}

void AChickenMan::CheckDistance(float DistM)
{
	if (DistM <= 0.0f)
		Eliminate();

}

void AChickenMan::Eliminate()
{
	if (bEliminated) return;
	bEliminated = true;

	UE_LOG(LogTemp, Warning, TEXT("Eliminated!"));


	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		DisableInput(PC);
	{
		APlayerController* PC0 = UGameplayStatics::GetPlayerController(this, 0);
		if (PC0)
		{
			FVector CamLoc; FRotator CamRot;
			PC0->GetPlayerViewPoint(CamLoc, CamRot);


			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACameraActor* StaticCam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, CamRot, Params);


			if (StaticCam && PC0->PlayerCameraManager)
				StaticCam->GetCameraComponent()->FieldOfView =
				PC0->PlayerCameraManager->GetFOVAngle();


			PC0->SetViewTargetWithBlend(StaticCam, 0.0f);
		}
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



