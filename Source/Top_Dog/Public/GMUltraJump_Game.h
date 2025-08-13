#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GMUltraJump_Game.generated.h"

class AUltra_Jump_Boy;
class UUserWidget;
class UTextBlock;

UCLASS()
class TOP_DOG_API AGMUltraJump_Game : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	void TickTimer();
	void EndMiniGame();

private:

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AUltra_Jump_Boy> JumpBoyClass;

	UPROPERTY()
	TArray<AUltra_Jump_Boy*> AllPlayers;

	UPROPERTY()
	TMap<APawn*, float> SpawnZMap;


	UPROPERTY(EditAnywhere, Category = "Match")
	int32 MatchSeconds = 45;

	UPROPERTY(VisibleAnywhere, Category = "Match")
	int32 RemainingTime = 0;

	UPROPERTY()
	FTimerHandle TimerHandle;

	UPROPERTY()
	FTimerHandle EndHandle;

	UPROPERTY()
	bool bMiniGameEnded = false;


	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> TimerWidgetClass;

	UPROPERTY()
	UUserWidget* TimerWidget = nullptr;

	UPROPERTY()
	UTextBlock* TimerTextBlock = nullptr;
};
