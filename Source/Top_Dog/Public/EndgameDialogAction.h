#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "EndgameDialogAction.generated.h"

class UUserWidget;
class ACineCameraActor;
class APlayerController;
class UInputComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEndgameFinished, AActor*, WinnerCharacter, FName, WinnerID);

UCLASS()
class TOP_DOG_API UEndgameDialogAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Lanza la secuencia de fin de juego. */
	UFUNCTION(BlueprintCallable, Category = "Dialog|Endgame",
		meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UEndgameDialogAction* PlayEndgameDialog(
		UObject* WorldContextObject,
		UUserWidget* DialogWidget,
		const TArray<AActor*>& Characters,
		const TArray<AActor*>& Checkpoints,
		ACineCameraActor* CineCamera,
		float BlendTime /*=0.0f*/,
		float AutoCloseSeconds /*=10.0f*/
	);

	/** Se emite al finalizar la secuencia. */
	UPROPERTY(BlueprintAssignable)
	FOnEndgameFinished OnFinished;

	// --- Textos (puedes cambiarlos por defecto si quieres) ---
	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText IntroText = FText::FromString(TEXT(
		"It is Time! These chumps fought tooth and nail for it.\n"
		"So, let’s find out who is going to stop being a loser and become the TOP DOG!!!"));

	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText TieMedalsText = FText::FromString(TEXT(
		"Huh?!?! Two or more players have the same number of medals???\n"
		"Well, guess will have to decide the winner by who has the most dollars out of them!"));

	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText TieDollarsText = FText::FromString(TEXT(
		"What?!?!?! Another Tie?!?!\n"
		"This is getting ridiculous, alright then I’ll just have to choose the winner myself. "
		"Now let’s see who I like most… Ok got it!"
	));

	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText ClosingText = FText::FromString(TEXT(
		"Thank you all for watching! Hope to see you again! Bye!!!"));

protected:
	// Entrada
	UPROPERTY() UObject* WorldContext = nullptr;
	UPROPERTY() UUserWidget* Dialog = nullptr;
	UPROPERTY() ACineCameraActor* CutsceneCamera = nullptr;
	UPROPERTY() APlayerController* PC = nullptr;

	TArray<TWeakObjectPtr<AActor>> CharactersRef;
	TArray<TWeakObjectPtr<AActor>> CheckpointsRef;

	float ViewBlend = 0.f;
	float AutoCloseOkSeconds = 10.f;

	// Secuencia
	TArray<FText> Messages;
	int32 MsgIndex = INDEX_NONE;

	// Ganador
	UPROPERTY() AActor* WinnerActor = nullptr;
	FName WinnerID = NAME_None;

	// Input/Timers (para avanzar mensajes OK)
	UPROPERTY() UInputComponent* TempInput = nullptr;
	FTimerHandle AutoCloseTimer;

	// ---- Flujo principal ----
	void Start();
	void SetupCameraAndTeleport();
	void BuildSequence();

	// Mostrar mensajes
	void ShowNext();
	void BP_OpenMessage(const FText& InText, bool bShowChoices);
	void BindWidgetOnResult();
	UFUNCTION() void HandleDialogResult_Byte(uint8 Result);
	void StartAutoCloseTimer();
	void StopAutoCloseTimer();
	void EnableAnyKeyCapture();
	void DisableAnyKeyCapture();
	void OnAnyKeyPressed(FKey Key);

	// Utilidades de datos
	struct FCandidate
	{
		AActor* Actor = nullptr;
		FName   ID = NAME_None;
		int32   Medals = 0;
		int32   Dollars = 0;
	};
	void CollectCandidates(TArray<FCandidate>& Out);
	static bool GetNameProp(AActor* Obj, const FName& Prop, FName& Out);
	static bool GetIntProp(AActor* Obj, const FName& Prop, int32& Out);
	static AActor* FindCheckpointByID(const TArray<TWeakObjectPtr<AActor>>& Pool, const FName& ID);

	// Decidir ganador
	void DecideWinner();
	// xoroshiro128+ (para desempate final)
	void SeedRNG();
	uint64 Next64();
	int32 RandIndex(int32 MaxExclusive);

	uint64 RNG[2] = { 0,0 };

	// Fin
	void Finish();
};
