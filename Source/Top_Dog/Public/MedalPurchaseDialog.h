#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "MedalPurchaseDialog.generated.h"

// Estados internos del flujo
UENUM()
enum class EMedalDialogPhase : uint8
{
	None,
	ShowingNotEnough,   // OK-only
	ShowingCongrats,    // OK-only
	AskingYesNo,        // YES/NO
	ShowingFinalYes,    // OK-only
	ShowingFinalNo      // OK-only
};

// Dispatcher de salida (TRUE si compró)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMedalDialogFinished, bool, bBought);

/**
 * Nodo async que orquesta los mensajes usando tu Widget Blueprint existente.
 * El widget YA debe estar creado y añadido al viewport.
 */
UCLASS()
class TOP_DOG_API UMedalPurchaseDialog : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dialog|Medal",
		meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UMedalPurchaseDialog* ShowMedalPurchaseDialog(
		UObject* WorldContextObject, class UUserWidget* DialogWidget,
		bool bHasEnoughMoney, bool bIsBot);

	UPROPERTY(BlueprintAssignable)
	FOnMedalDialogFinished OnFinished;

	// ---- Textos configurables ----
	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText NotEnoughMoneyText = FText::FromString(TEXT("Sorry, no broke boys allowed!"));

	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText CongratsText = FText::FromString(TEXT("Yay! You made it!"));

	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText QuestionText = FText::FromString(TEXT("Want to buy a medal for $200?"));

	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText FinalYesText = FText::FromString(TEXT("Congrats! Good luck out there!"));

	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Text")
	FText FinalNoText = FText::FromString(TEXT("What?! Why not?! Are you stupid?!!!"));

	/** Segundos para auto-cerrar pasos de OK (los YES/NO NO autocerran). */
	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Timing")
	float AutoCloseOkSeconds = 10.f;

protected:
	// ---- Estado / refs ----
	UPROPERTY() UObject* WorldContext = nullptr;
	UPROPERTY() class UUserWidget* Dialog = nullptr;
	UPROPERTY() class APlayerController* PC = nullptr;
	UPROPERTY() class UInputComponent* TempInput = nullptr;

	EMedalDialogPhase Phase = EMedalDialogPhase::None;
	bool bBought = false;
	bool bIsBotMode = false;

	FTimerHandle AutoCloseTimer;

	// ---- Lógica principal ----
	void Start(bool bHasEnoughMoney);

	/** Llama a la función BP "Open Message" (Text, bShowChoices). */
	void BP_OpenMessage(const FText& InText, bool bShowChoices);

	/** Llama a la función BP "Call On Result" (Result uint8). */
	void BP_CallOnResult(uint8 ResultByte);

	/** Se engancha al dispatcher BP "OnResult" del widget (firma uint8 o enum byte). */
	void BindWidgetOnResult();

	/** Handler: 0=Ok, 1=Yes, 2=No */
	UFUNCTION()
	void HandleDialogResult_Byte(uint8 Result);

	// AnyKey (solo pasos OK)
	void EnableAnyKeyCapture();
	void DisableAnyKeyCapture();
	void OnAnyKeyPressed();

	// Timers de autocierre (solo OK)
	void StartAutoCloseTimer();
	void StopAutoCloseTimer();
	void AutoCloseNow();

	// Estados
	void GoTo_NotEnough();
	void GoTo_Congrats();
	void GoTo_Question();
	void GoTo_FinalYes();
	void GoTo_FinalNo();

	// Fin
	void FinishAndCleanup(bool bDidBuy);
};
