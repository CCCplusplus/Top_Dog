
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AGMChicken_Game.generated.h"

class AChickenMan;


UCLASS()
class TOP_DOG_API AAGMChicken_Game : public AGameModeBase
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;


	void OnPlayerEliminated(AChickenMan* Victim);

	void NotifyPlayerFinished(AChickenMan* FinishedPawn);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AChickenMan> ChickenManClass;


	UPROPERTY()
	TArray<AChickenMan*> AllPlayers;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AActor> CrusherClass;


	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float CrusherSpawnDistance = 500.f;


	int32 FinishedCount = 0;

private:

	bool        bMiniGameEnded = false;

	FTimerHandle EndHandle;

	void       EndMiniGame();
};
