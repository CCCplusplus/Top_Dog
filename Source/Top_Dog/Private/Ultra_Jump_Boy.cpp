// Fill out your copyright notice in the Description page of Project Settings.


#include "Ultra_Jump_Boy.h"

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


	// 1) Añadir el Mapping Context al Enhanced Input Subsystem
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
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
