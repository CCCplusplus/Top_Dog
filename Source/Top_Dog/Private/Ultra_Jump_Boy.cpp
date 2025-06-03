// Fill out your copyright notice in the Description page of Project Settings.


#include "Ultra_Jump_Boy.h"
#include <Kismet/GameplayStatics.h>

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

// Called when the game starts or when spawned
void AUltra_Jump_Boy::BeginPlay()
{
	Super::BeginPlay();

	SpringArm->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	InitialCameraLocation = SpringArm->GetComponentLocation();
	InitialCharacterZ = GetActorLocation().Z;

	GetCharacterMovement()->JumpZVelocity = jumpHeight;

	// Obtén referencia al componente de movimiento
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	if (MoveComp)
	{
		// Permite control total en el aire (0.0 = no control; 1.0 = control completo)
		MoveComp->AirControl = 1.0f;

		// Opcional: refuerza el efecto si acelera mucho
		MoveComp->AirControlBoostMultiplier = 1.0f;
		MoveComp->AirControlBoostVelocityThreshold = 0.0f;

		// (Bonus) Quita fricción en aire para que no desacelere
		MoveComp->BrakingFrictionFactor = 0.0f;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (TimerWidgetClass)
	{
		TimerWidget = CreateWidget<UUserWidget>(GetWorld(), TimerWidgetClass);
		if (TimerWidget)
		{
			TimerWidget->AddToViewport();
			TimerTextBlock = Cast<UTextBlock>(TimerWidget->GetWidgetFromName(TEXT("Timer"))); // nombre exacto en UMG
			RefreshTimerUI();

			// cada segundo
			GetWorldTimerManager().SetTimer(TimerHandle, this, &AUltra_Jump_Boy::UpdateTimer, 1.0f, true);
		}
	}

	if (HeightWidgetClass)
	{
		HeightWidget = CreateWidget<UUserWidget>(GetWorld(), HeightWidgetClass);
		if (HeightWidget)
		{
			HeightWidget->AddToViewport();
			HeightTextBlock = Cast<UTextBlock>(HeightWidget->GetWidgetFromName(TEXT("HeightValue"))); // nombre exacto en UMG
			// valor inicial opcional:
			if (HeightTextBlock)
				HeightTextBlock->SetText(FText::FromString(TEXT("0.00 M")));
		}
	}
	
}

// Called every frame
void AUltra_Jump_Boy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float CurrentZ = GetActorLocation().Z;
	float DeltaZ = CurrentZ - InitialCharacterZ;

	// Nueva posición de la cámara: misma X/Y, pero Z adaptada
	FVector NewCamLoc = InitialCameraLocation + FVector(0.f, 0.f, DeltaZ);
	SpringArm->SetWorldLocation(NewCamLoc);

	if (HeightTextBlock)
	{
		float DeltaCm = CurrentZ - InitialCharacterZ;
		float Meters = DeltaCm * 0.01f;
		HeightTextBlock->SetText(
			FText::FromString(FString::Printf(TEXT("%.2f M"), Meters))
		);
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

void AUltra_Jump_Boy::UpdateTimer()
{
	RemainingTime = FMath::Max(RemainingTime - 1, 0);
	RefreshTimerUI();

	if (RemainingTime == 0)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		// Poner resultados aqui. 
		// por ahora detenemos movimiento y salto
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->SetMovementMode(MOVE_None);

		//agregar delay de 2 segundos y cambiar de nivel a "ThirdPersonMap"
		// utilizando,UGameplayStatics::OpenLevel()
		FTimerHandle DelayHandle;
		GetWorldTimerManager().SetTimer(DelayHandle, [this]()
			{
				UGameplayStatics::OpenLevel(this, FName("ThirdPersonMap"));
			}, 2.0f, false);

	}
}

void AUltra_Jump_Boy::RefreshTimerUI()
{
	if (TimerTextBlock)
		TimerTextBlock->SetText(FText::AsNumber(RemainingTime));
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
