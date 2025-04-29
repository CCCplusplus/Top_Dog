// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "JumpBoy.generated.h"

//Jumpboy is a Spawn for a minigame where you control a chatacter that jumps from one platform to another with a front facing camera
//it needs to jump from left to right if its in the left lane it can't jump left it can only jump in place or to the right
//same thing if it's in the right lane it can only jump in place or to the left.
//if there is a platform above it jumps on top of it, if there isn't he jumps in place
//if he jumps to the oposite side and there's a platform he will land on top of it if there isn't he will fall to the closet platform on that side.


// Move the UPROPERTY declarations inside the AJumpBoy class definition.  
UCLASS()  
class TOP_DOG_API AJumpBoy : public APawn  
{  
   GENERATED_BODY()  

public:  
   // Sets default values for this pawn's properties  
   AJumpBoy();  

protected:  
   // Called when the game starts or when spawned  
   virtual void BeginPlay() override;  

public:      
   // Called every frame  
   virtual void Tick(float DeltaTime) override;  

   // Called to bind functionality to input  
   virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;  


};
