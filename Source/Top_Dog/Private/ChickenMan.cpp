// Fill out your copyright notice in the Description page of Project Settings.


#include "ChickenMan.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"

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
}

// Called when the game starts or when spawned
void AChickenMan::BeginPlay()
{
	Super::BeginPlay();

	//Crusher is = to the actor with the tag "Crusher"
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Crusher"), FoundActors);
	if (FoundActors.Num() > 0)
		CrusherActor = FoundActors[0];
	else 
		UE_LOG(LogTemp, Warning, TEXT("Crusher actor not found"));
	

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		// add Enhanced Input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsys =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			Subsys->AddMappingContext(DefaultMappingContext, 0);
		
	}

	if (DistanceWidgetClass)
	{
		DistanceWidget = CreateWidget<UUserWidget>(GetWorld(), DistanceWidgetClass);
		if (DistanceWidget)
		{
			DistanceWidget->AddToViewport();

			// 2) Buscamos el TextBlock por nombre (tal como lo marcaste en UMG)
			DistanceTextBlock = Cast<UTextBlock>(
				DistanceWidget->WidgetTree->FindWidget(TEXT("HeightValue"))
			);
		}
	}
}

// Called every frame
void AChickenMan::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!DistanceTextBlock || !CrusherActor)
		return;

	const FVector PlayerLoc = GetActorLocation();

	
	FVector ClosestPoint = CrusherActor->GetActorLocation();
	if (UPrimitiveComponent* Prim =
		CrusherActor->FindComponentByClass<UPrimitiveComponent>())
	{
		
		FVector OutPoint;
		if (Prim->GetClosestPointOnCollision(PlayerLoc, OutPoint))
		{
			ClosestPoint = OutPoint;
		}
		
	}

	
	float DistCm = FVector::Dist(PlayerLoc, ClosestPoint);
	float DistM = DistCm / 100.f;
	float Rounded = FMath::RoundToFloat(DistM * 100.f) / 100.f;

	
	DistanceTextBlock->SetText(FText::AsNumber(Rounded));
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

	// 2) Calcular nuevo yaw: si estaba en -90, pasa a +90; si no, pasa a -90
	const float NewYaw = FMath::IsNearlyEqual(RelRot.Yaw, -90.f, 1.f)
		? 90.f
		: -90.f;

	RelRot.Yaw = NewYaw;
	GetMesh()->SetRelativeRotation(RelRot);

	// 4) Avanzar unos pasos
	FVector Dir = GetActorForwardVector();
	LaunchCharacter(-Dir * 400.f, true, true);

	hasturned = true;

	if (CrusherActor)
	{
		if (APawn* CrusherPawn = Cast<APawn>(CrusherActor))
		{
			if (AAIController* AICon = Cast<AAIController>(CrusherPawn->GetController()))
			{
				if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
				{
					BB->SetValueAsBool(TEXT("StopMoving"), true);
				}
			}
		}
	}

	FTimerHandle DelayHandle;
	GetWorldTimerManager().SetTimer(DelayHandle, [this]()
		{
			UGameplayStatics::OpenLevel(this, FName("ThirdPersonMap"));
		}, 2.0f, false);
}
