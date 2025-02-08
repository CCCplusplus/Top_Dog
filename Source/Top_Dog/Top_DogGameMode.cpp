// Copyright Epic Games, Inc. All Rights Reserved.

#include "Top_DogGameMode.h"
#include "Top_DogCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATop_DogGameMode::ATop_DogGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
