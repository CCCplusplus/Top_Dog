#include "MedalPurchaseDialog.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"

#include "UObject/UnrealType.h"  // FProperty tipos
#include "UObject/Field.h"       // FindFProperty
#include "Engine/World.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"

UMedalPurchaseDialog* UMedalPurchaseDialog::ShowMedalPurchaseDialog(
	UObject* WorldContextObject, UUserWidget* DialogWidget,
	bool bHasEnoughMoney, bool bIsBot)
{
	UMedalPurchaseDialog* Action = NewObject<UMedalPurchaseDialog>();
	Action->WorldContext = WorldContextObject;
	Action->Dialog = DialogWidget;
	Action->bIsBotMode = bIsBot;

	if (UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		Action->PC = UGameplayStatics::GetPlayerController(W, 0);
	}

	Action->RegisterWithGameInstance(WorldContextObject);
	Action->Start(bHasEnoughMoney);
	return Action;
}

void UMedalPurchaseDialog::Start(bool bHasEnoughMoney)
{
	if (!Dialog)
	{
		FinishAndCleanup(false);
		return;
	}

	BindWidgetOnResult();

	if (!bHasEnoughMoney)
	{
		GoTo_NotEnough();
	}
	else
	{
		GoTo_Congrats();
	}
}

void UMedalPurchaseDialog::BindWidgetOnResult()
{
	if (!Dialog) return;

	// Creamos un delegate dinámico hacia nuestra función UFUNCTION
	FScriptDelegate Dyn;
	Dyn.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UMedalPurchaseDialog, HandleDialogResult_Byte));

	// 1) Propiedad delegate externa
	if (FMulticastDelegateProperty* DelProp =
		FindFProperty<FMulticastDelegateProperty>(Dialog->GetClass(), TEXT("OnResult")))
	{
		const FMulticastScriptDelegate* ConstDel =
			DelProp->GetMulticastDelegate(DelProp->ContainerPtrToValuePtr<void>(Dialog));
		if (ConstDel)
		{
			FMulticastScriptDelegate* Del = const_cast<FMulticastScriptDelegate*>(ConstDel);
			Del->AddUnique(Dyn);
			return;
		}
	}

	// 2) Propiedad inline (lo más común en Widgets BP)
	if (FMulticastInlineDelegateProperty* InlineProp =
		FindFProperty<FMulticastInlineDelegateProperty>(Dialog->GetClass(), TEXT("OnResult")))
	{
		const FMulticastScriptDelegate* ConstDel =
			InlineProp->GetMulticastDelegate(InlineProp->ContainerPtrToValuePtr<void>(Dialog));
		if (ConstDel)
		{
			FMulticastScriptDelegate* Del = const_cast<FMulticastScriptDelegate*>(ConstDel);
			Del->AddUnique(Dyn);
		}
	}
}

void UMedalPurchaseDialog::BP_OpenMessage(const FText& InText, bool bShowChoices)
{
	if (!Dialog) return;

	static const FName FuncName(TEXT("Open Message"));
	if (UFunction* Fn = Dialog->FindFunction(FuncName))
	{
		struct FOpenMessage_Params { FText Message; bool bShowChoices; };
		FOpenMessage_Params P; P.Message = InText; P.bShowChoices = bShowChoices;
		Dialog->ProcessEvent(Fn, &P);
	}
}

void UMedalPurchaseDialog::BP_CallOnResult(uint8 ResultByte)
{
	if (!Dialog) return;

	static const FName FuncName(TEXT("Call On Result"));
	if (UFunction* Fn = Dialog->FindFunction(FuncName))
	{
		struct FCallOnResult_Params { uint8 Result; };
		FCallOnResult_Params P; P.Result = ResultByte;
		Dialog->ProcessEvent(Fn, &P);
	}
}

void UMedalPurchaseDialog::EnableAnyKeyCapture()
{
	if (!PC) return;

	if (!TempInput)
	{
		TempInput = NewObject<UInputComponent>(PC, TEXT("MedalDialog_Input"));
		TempInput->bBlockInput = false;
		TempInput->Priority = 10;
		TempInput->RegisterComponent();
	}

	TempInput->ClearActionBindings();
	TempInput->AxisBindings.Reset();
	TempInput->KeyBindings.Reset();

	const FInputChord Chord(EKeys::AnyKey, /*bShift=*/false, /*bCtrl=*/false, /*bAlt=*/false, /*bCmd=*/false);
	FInputKeyBinding AnyKey(Chord, IE_Pressed);
	AnyKey.bConsumeInput = false;
	AnyKey.KeyDelegate.GetDelegateForManualSet()
		.BindUObject(this, &UMedalPurchaseDialog::OnAnyKeyPressed);

	TempInput->KeyBindings.Add(MoveTemp(AnyKey));
	PC->PushInputComponent(TempInput);
}

void UMedalPurchaseDialog::DisableAnyKeyCapture()
{
	if (PC && TempInput)
	{
		PC->PopInputComponent(TempInput);
	}
}

void UMedalPurchaseDialog::OnAnyKeyPressed()
{
	switch (Phase)
	{
	case EMedalDialogPhase::ShowingNotEnough:
	case EMedalDialogPhase::ShowingCongrats:
	case EMedalDialogPhase::ShowingFinalYes:
	case EMedalDialogPhase::ShowingFinalNo:
		HandleDialogResult_Byte(0); // OK
		break;
	default:
		break;
	}
}

void UMedalPurchaseDialog::StartAutoCloseTimer()
{
	if (UWorld* W = GetWorld())
	{
		StopAutoCloseTimer();
		W->GetTimerManager().SetTimer(
			AutoCloseTimer, this, &UMedalPurchaseDialog::AutoCloseNow, AutoCloseOkSeconds, false);
	}
}

void UMedalPurchaseDialog::StopAutoCloseTimer()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AutoCloseTimer);
	}
}

void UMedalPurchaseDialog::AutoCloseNow()
{
	HandleDialogResult_Byte(0); // OK
}

void UMedalPurchaseDialog::HandleDialogResult_Byte(uint8 Result)
{
	switch (Phase)
	{
	case EMedalDialogPhase::ShowingNotEnough:
		FinishAndCleanup(false);
		break;

	case EMedalDialogPhase::ShowingCongrats:
		GoTo_Question();
		break;

	case EMedalDialogPhase::AskingYesNo:
		if (Result == 1)      GoTo_FinalYes(); // YES
		else if (Result == 2) GoTo_FinalNo();  // NO
		break;

	case EMedalDialogPhase::ShowingFinalYes:
		FinishAndCleanup(true);
		break;

	case EMedalDialogPhase::ShowingFinalNo:
		FinishAndCleanup(false);
		break;

	default: break;
	}
}

void UMedalPurchaseDialog::GoTo_NotEnough()
{
	Phase = EMedalDialogPhase::ShowingNotEnough;
	bBought = false;

	BP_OpenMessage(NotEnoughMoneyText, /*ShowChoices=*/false);
	EnableAnyKeyCapture();
	StartAutoCloseTimer();
}

void UMedalPurchaseDialog::GoTo_Congrats()
{
	Phase = EMedalDialogPhase::ShowingCongrats;
	bBought = false;

	BP_OpenMessage(CongratsText, /*ShowChoices=*/false);
	EnableAnyKeyCapture();
	StartAutoCloseTimer();
}

void UMedalPurchaseDialog::GoTo_Question()
{
	Phase = EMedalDialogPhase::AskingYesNo;

	DisableAnyKeyCapture();
	StopAutoCloseTimer();
	BP_OpenMessage(QuestionText, /*ShowChoices=*/true);

	if (bIsBotMode)
	{
		if (UWorld* W = GetWorld())
		{
			FTimerHandle Tmp;
			W->GetTimerManager().SetTimer(
				Tmp, FTimerDelegate::CreateUObject(this, &UMedalPurchaseDialog::BP_CallOnResult, (uint8)1),
				0.05f, false); // auto YES
		}
	}
}

void UMedalPurchaseDialog::GoTo_FinalYes()
{
	Phase = EMedalDialogPhase::ShowingFinalYes;
	bBought = true;

	BP_OpenMessage(FinalYesText, /*ShowChoices=*/false);
	EnableAnyKeyCapture();
	StartAutoCloseTimer();
}

void UMedalPurchaseDialog::GoTo_FinalNo()
{
	Phase = EMedalDialogPhase::ShowingFinalNo;
	bBought = false;

	BP_OpenMessage(FinalNoText, /*ShowChoices=*/false);
	EnableAnyKeyCapture();
	StartAutoCloseTimer();
}

void UMedalPurchaseDialog::FinishAndCleanup(bool bDidBuy)
{
	StopAutoCloseTimer();
	DisableAnyKeyCapture();

	if (Dialog)
	{
		Dialog->RemoveFromParent();
	}

	OnFinished.Broadcast(bDidBuy);
	SetReadyToDestroy();
}
