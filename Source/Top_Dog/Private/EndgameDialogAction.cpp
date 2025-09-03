#include "EndgameDialogAction.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "CineCameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "UObject/UnrealType.h"
#include "UObject/Field.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"

UEndgameDialogAction* UEndgameDialogAction::PlayEndgameDialog(
	UObject* WorldContextObject,
	UUserWidget* DialogWidget,
	const TArray<AActor*>& Characters,
	const TArray<AActor*>& Checkpoints,
	ACineCameraActor* CineCamera,
	float BlendTime,
	float AutoCloseSeconds
)
{
	UEndgameDialogAction* Action = NewObject<UEndgameDialogAction>();
	Action->WorldContext = WorldContextObject;
	Action->Dialog = DialogWidget;
	Action->CutsceneCamera = CineCamera;
	Action->ViewBlend = BlendTime;
	Action->AutoCloseOkSeconds = AutoCloseSeconds;

	for (AActor* A : Characters) { Action->CharactersRef.Add(A); }
	for (AActor* A : Checkpoints) { Action->CheckpointsRef.Add(A); }

	if (UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		Action->PC = UGameplayStatics::GetPlayerController(W, 0);
	}

	Action->RegisterWithGameInstance(WorldContextObject);
	Action->Start();
	return Action;
}

// ----------------- FLOW -----------------

void UEndgameDialogAction::Start()
{
	if (!Dialog)
	{
		Finish();
		return;
	}

	BindWidgetOnResult();
	SeedRNG();

	SetupCameraAndTeleport();
	DecideWinner();
	BuildSequence();

	MsgIndex = -1;
	ShowNext();
}

void UEndgameDialogAction::SetupCameraAndTeleport()
{
	// Fijar cámara
	if (PC && CutsceneCamera)
	{
		PC->SetViewTargetWithBlend(CutsceneCamera, ViewBlend);
	}

	// Teletransportar cada Character a su Checkpoint por ID
	for (const TWeakObjectPtr<AActor>& C : CharactersRef)
	{
		AActor* Char = C.Get();
		if (!Char) continue;

		FName CharID = NAME_None;
		if (!GetNameProp(Char, TEXT("ID"), CharID))
			continue;

		if (AActor* CP = FindCheckpointByID(CheckpointsRef, CharID))
		{
			const FTransform T = CP->GetActorTransform();
			Char->SetActorLocationAndRotation(T.GetLocation(), T.GetRotation().Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void UEndgameDialogAction::BuildSequence()
{
	TArray<FText> PreBuilt = MoveTemp(Messages);

	Messages.Reset();

	Messages.Add(IntroText);

	if (PreBuilt.Num() > 0)
		Messages.Append(PreBuilt);

	const FText WinnerLine = FText::FromString(
		FString::Printf(TEXT("And the winner is…\n\"%s\" Congratulations you are the TOP DOG!!!"),
			*WinnerID.ToString())
	);
	Messages.Add(WinnerLine);

	Messages.Add(ClosingText);
}


void UEndgameDialogAction::ShowNext()
{
	StopAutoCloseTimer();
	DisableAnyKeyCapture();

	MsgIndex++;
	if (!Messages.IsValidIndex(MsgIndex))
	{
		Finish();
		return;
	}


	BP_OpenMessage(Messages[MsgIndex], false);
	EnableAnyKeyCapture();
	StartAutoCloseTimer();
}

void UEndgameDialogAction::BP_OpenMessage(const FText& InText, bool bShowChoices)
{
	if (!Dialog) return;

	static const FName FuncName(TEXT("Open Message"));
	if (UFunction* Fn = Dialog->FindFunction(FuncName))
	{
		struct FParams { FText Message; bool bShowChoices; };
		FParams P; P.Message = InText; P.bShowChoices = bShowChoices;
		Dialog->ProcessEvent(Fn, &P);
	}
}

void UEndgameDialogAction::BindWidgetOnResult()
{
	if (!Dialog) return;

	// Propiedad "OnResult" creada por el BP (Dynamic Multicast)
	if (const FMulticastDelegateProperty* DelProp =
		FindFProperty<FMulticastDelegateProperty>(Dialog->GetClass(), TEXT("OnResult")))
	{
		// En 5.x GetMulticastDelegate puede devolver puntero const; quita const para poder añadir
		FMulticastScriptDelegate* Del = const_cast<FMulticastScriptDelegate*>(
			DelProp->GetMulticastDelegate(DelProp->ContainerPtrToValuePtr<void>(Dialog)));

		if (Del)
		{
			FScriptDelegate D;
			D.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UEndgameDialogAction, HandleDialogResult_Byte));
			Del->Add(D); // <- Add con FScriptDelegate (no AddUFunction / AddDynamic)
		}
	}
	else if (const FMulticastInlineDelegateProperty* InlineProp =
		FindFProperty<FMulticastInlineDelegateProperty>(Dialog->GetClass(), TEXT("OnResult")))
	{
		FMulticastScriptDelegate* Del = const_cast<FMulticastScriptDelegate*>(
			InlineProp->GetMulticastDelegate(InlineProp->ContainerPtrToValuePtr<void>(Dialog)));

		if (Del)
		{
			FScriptDelegate D;
			D.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UEndgameDialogAction, HandleDialogResult_Byte));
			Del->Add(D);
		}
	}
}


void UEndgameDialogAction::HandleDialogResult_Byte(uint8 )
{
	ShowNext();
}

void UEndgameDialogAction::StartAutoCloseTimer()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(AutoCloseTimer, this, &UEndgameDialogAction::ShowNext, AutoCloseOkSeconds, false);
	}
}

void UEndgameDialogAction::StopAutoCloseTimer()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AutoCloseTimer);
	}
}

void UEndgameDialogAction::EnableAnyKeyCapture()
{
	if (!PC) return;

	if (!TempInput)
	{
		TempInput = NewObject<UInputComponent>(PC, TEXT("EndgameDialog_Input"));
		TempInput->RegisterComponent();
		TempInput->bBlockInput = false;
		TempInput->Priority = 10;
	}

	TempInput->ClearActionBindings();
	TempInput->KeyBindings.Empty();

	FInputKeyBinding AnyKey(EKeys::AnyKey, IE_Pressed);
	AnyKey.bConsumeInput = false;
	AnyKey.KeyDelegate.GetDelegateForManualSet()
		.BindLambda([this]()
			{
				this->OnAnyKeyPressed(FKey());
			});

	TempInput->KeyBindings.Add(AnyKey);

	PC->PushInputComponent(TempInput);
}

void UEndgameDialogAction::DisableAnyKeyCapture()
{
	if (PC && TempInput)
	{
		PC->PopInputComponent(TempInput);
	}
}

void UEndgameDialogAction::OnAnyKeyPressed(FKey )
{

	ShowNext();
}



bool UEndgameDialogAction::GetNameProp(AActor* Obj, const FName& Prop, FName& Out)
{
	if (!Obj) return false;
	if (FNameProperty* P = FindFProperty<FNameProperty>(Obj->GetClass(), Prop))
	{
		Out = *P->ContainerPtrToValuePtr<FName>(Obj);
		return true;
	}
	return false;
}

bool UEndgameDialogAction::GetIntProp(AActor* Obj, const FName& Prop, int32& Out)
{
	if (!Obj) return false;

	if (FIntProperty* P = FindFProperty<FIntProperty>(Obj->GetClass(), Prop))
	{
		Out = *P->ContainerPtrToValuePtr<int32>(Obj);
		return true;
	}
	// Si fuera float por accidente, permitir:
	if (FFloatProperty* PF = FindFProperty<FFloatProperty>(Obj->GetClass(), Prop))
	{
		Out = FMath::RoundToInt(*PF->ContainerPtrToValuePtr<float>(Obj));
		return true;
	}
	return false;
}

AActor* UEndgameDialogAction::FindCheckpointByID(const TArray<TWeakObjectPtr<AActor>>& Pool, const FName& ID)
{
	for (const TWeakObjectPtr<AActor>& C : Pool)
	{
		AActor* A = C.Get();
		if (!A) continue;

		FName CID = NAME_None;
		if (GetNameProp(A, TEXT("ID"), CID) && CID == ID)
		{
			return A;
		}
	}
	return nullptr;
}

void UEndgameDialogAction::CollectCandidates(TArray<FCandidate>& Out)
{
	Out.Reset();
	for (const TWeakObjectPtr<AActor>& C : CharactersRef)
	{
		FCandidate X;
		X.Actor = C.Get();
		if (!X.Actor) continue;

		GetNameProp(X.Actor, TEXT("ID"), X.ID);
		GetIntProp(X.Actor, TEXT("Medals"), X.Medals);
		GetIntProp(X.Actor, TEXT("Dollars"), X.Dollars);

		Out.Add(X);
	}
}

void UEndgameDialogAction::DecideWinner()
{
	TArray<FCandidate> Cands;
	CollectCandidates(Cands);
	if (Cands.Num() == 0)
	{
		WinnerActor = nullptr;
		WinnerID = TEXT("Unknown");
		return;
	}


	int32 MaxMedals = TNumericLimits<int32>::Lowest();
	for (const auto& C : Cands) { MaxMedals = FMath::Max(MaxMedals, C.Medals); }

	TArray<FCandidate> ByMedals;
	for (const auto& C : Cands) if (C.Medals == MaxMedals) ByMedals.Add(C);

	bool bTieMedals = (ByMedals.Num() > 1);


	TArray<FCandidate> ByDollars = ByMedals;
	bool bTieDollars = false;
	if (bTieMedals)
	{
		int32 MaxDol = TNumericLimits<int32>::Lowest();
		for (const auto& C : ByDollars) { MaxDol = FMath::Max(MaxDol, C.Dollars); }

		TArray<FCandidate> TopDol;
		for (const auto& C : ByDollars) if (C.Dollars == MaxDol) TopDol.Add(C);

		bTieDollars = (TopDol.Num() > 1);

		if (!bTieDollars)
		{
			ByDollars = TopDol;
		}
		else
		{
			const int32 Pick = RandIndex(TopDol.Num());
			ByDollars = { TopDol[Pick] };

			Messages.Add(TieMedalsText);
			Messages.Add(TieDollarsText);
		}
	}


	if (bTieMedals && !bTieDollars)
	{
		Messages.Add(TieMedalsText);
	}


	const FCandidate& W = (bTieMedals ? ByDollars[0] : ByMedals[0]);
	WinnerActor = W.Actor;
	WinnerID = W.ID;
}


static uint64 SplitMix64(uint64& X)
{
	uint64 Z = (X += 0x9E3779B97F4A7C15ull);
	Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ull;
	Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBull;
	return Z ^ (Z >> 31);
}

void UEndgameDialogAction::SeedRNG()
{
	uint64 Seed = (uint64)FDateTime::Now().GetTicks() ^ (uint64)this;
	RNG[0] = SplitMix64(Seed);
	RNG[1] = SplitMix64(Seed);
}

static FORCEINLINE uint64 ROTL(const uint64 x, int k) { return (x << k) | (x >> (64 - k)); }

uint64 UEndgameDialogAction::Next64()
{
	const uint64 s0 = RNG[0];
	uint64 s1 = RNG[1];
	const uint64 result = s0 + s1;

	s1 ^= s0;
	RNG[0] = ROTL(s0, 55) ^ s1 ^ (s1 << 14);
	RNG[1] = ROTL(s1, 36);

	return result;
}

int32 UEndgameDialogAction::RandIndex(int32 MaxExclusive)
{
	if (MaxExclusive <= 1) return 0;
	return (int32)(Next64() % (uint64)MaxExclusive);
}

void UEndgameDialogAction::Finish()
{
	StopAutoCloseTimer();
	DisableAnyKeyCapture();

	OnFinished.Broadcast(WinnerActor, WinnerID);
	SetReadyToDestroy();
}
