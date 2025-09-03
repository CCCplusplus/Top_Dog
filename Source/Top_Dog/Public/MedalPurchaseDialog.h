#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "MedalPurchaseDialog.generated.h"


UENUM()
enum class EMedalDialogPhase : uint8
{
	None,
	ShowingNotEnough,
	ShowingCongrats, 
	AskingYesNo,     
	ShowingFinalYes, 
	ShowingFinalNo   
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMedalDialogFinished, bool, bBought);

/*
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


	UPROPERTY(EditDefaultsOnly, Category = "Dialog|Timing")
	float AutoCloseOkSeconds = 10.f;

protected:

	UPROPERTY() UObject* WorldContext = nullptr;
	UPROPERTY() class UUserWidget* Dialog = nullptr;
	UPROPERTY() class APlayerController* PC = nullptr;
	UPROPERTY() class UInputComponent* TempInput = nullptr;

	EMedalDialogPhase Phase = EMedalDialogPhase::None;
	bool bBought = false;
	bool bIsBotMode = false;

	FTimerHandle AutoCloseTimer;


	void Start(bool bHasEnoughMoney);


	void BP_OpenMessage(const FText& InText, bool bShowChoices);


	void BP_CallOnResult(uint8 ResultByte);


	void BindWidgetOnResult();


	UFUNCTION()
	void HandleDialogResult_Byte(uint8 Result);


	void EnableAnyKeyCapture();
	void DisableAnyKeyCapture();
	void OnAnyKeyPressed();


	void StartAutoCloseTimer();
	void StopAutoCloseTimer();
	void AutoCloseNow();


	void GoTo_NotEnough();
	void GoTo_Congrats();
	void GoTo_Question();
	void GoTo_FinalYes();
	void GoTo_FinalNo();

	void FinishAndCleanup(bool bDidBuy);
};
