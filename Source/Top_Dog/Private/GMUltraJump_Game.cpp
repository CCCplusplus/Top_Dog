#include "GMUltraJump_Game.h"
#include "Ultra_Jump_Boy.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"


static int32 ReadNumPlayersFromGIJump(const UWorld* World)
{
	if (!World) return 1;
	if (const UGameInstance* GI = World->GetGameInstance())
	{
		static const FName VarName(TEXT("NumberofPlayer"));
		if (const FIntProperty* P =
			CastField<FIntProperty>(GI->GetClass()->FindPropertyByName(VarName)))
			return FMath::Clamp(P->GetPropertyValue_InContainer(GI), 1, 4);
	}
	return 1;
}

static int32 GetLaneFromStartJump(const AActor* A)
{
	if (!A) return INDEX_NONE;
	for (const FName& Tag : A->Tags)
	{
		const FString S = Tag.ToString();
		if (S.StartsWith(TEXT("Start_")))
		{
			const int32 N = FCString::Atoi(*S.Mid(6));
			if (N >= 1 && N <= 4) return N - 1;
		}
	}
	return INDEX_NONE;
}


void AGMUltraJump_Game::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;

	FTransform LaneTransforms[4];
	bool       bLaneFound[4] = { false,false,false,false };

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const int32 Lane = GetLaneFromStartJump(*It);
		if (Lane != INDEX_NONE)
		{
			LaneTransforms[Lane] = It->GetActorTransform();
			bLaneFound[Lane] = true;
		}
	}
	for (int32 i = 0; i < 4; ++i)
	{
		if (!bLaneFound[i])
			LaneTransforms[i] = FTransform(FVector(i * 300.f, 0.f, 300.f));
	}

	const int32 NumHumans = FMath::Clamp(ReadNumPlayersFromGIJump(World), 1, 4);
	UE_LOG(LogTemp, Warning, TEXT("UltraJump NumHumans = %d"), NumHumans);

	AllPlayers.Empty();
	SpawnZMap.Empty();

	for (int32 i = 0; i < 4; ++i)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, i);
		if (!PC)
			PC = UGameplayStatics::CreatePlayer(World, i, true);
		if (!PC) continue;

		if (APawn* Old = PC->GetPawn())
			Old->Destroy();

		AUltra_Jump_Boy* JB = nullptr;
		if (ensure(JumpBoyClass))
			JB = World->SpawnActor<AUltra_Jump_Boy>(JumpBoyClass, LaneTransforms[i]);
		if (!JB) continue;

		PC->Possess(JB);
		PC->SetViewTarget(JB);

		const bool bIsHuman = (i < NumHumans);
		if (bIsHuman) PC->EnableInput(PC);
		else          PC->DisableInput(PC);

		JB->bIsBot = !bIsHuman;

		const FName PlayerTag = FName(*FString::Printf(TEXT("Player_%d"), i + 1));
		JB->Tags.AddUnique(PlayerTag);

		SpawnZMap.Add(JB, JB->GetActorLocation().Z);

		AllPlayers.Add(JB);
	}

	if (TimerWidgetClass)
	{
		TimerWidget = CreateWidget<UUserWidget>(GetWorld(), TimerWidgetClass);
		if (TimerWidget)
		{
			TimerWidget->AddToViewport(1000);

			// El UserWidget puede seguir llenando pantalla; centramos el TextBlock dentro
			TimerTextBlock = Cast<UTextBlock>(TimerWidget->WidgetTree->FindWidget(TEXT("Timer")));

			if (TimerTextBlock)
			{
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TimerTextBlock->Slot))
				{
					CanvasSlot->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));   // centro-arriba
					CanvasSlot->SetAlignment(FVector2D(0.5f, 0.f));           // pivot centrado
					CanvasSlot->SetPosition(FVector2D(0.f, 20.f));            // margen superior
					CanvasSlot->SetAutoSize(true);                            // tamaño al contenido
				}
			}

			// (Opcional) evita que tape clics
			TimerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

	}

	RemainingTime = MatchSeconds;
	if (TimerTextBlock)
		TimerTextBlock->SetText(FText::AsNumber(RemainingTime));

	GetWorldTimerManager().ClearTimer(TimerHandle);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AGMUltraJump_Game::TickTimer, 1.0f, true);
}

void AGMUltraJump_Game::TickTimer()
{
	if (bMiniGameEnded) return;

	RemainingTime = FMath::Max(RemainingTime - 1, 0);

	if (TimerTextBlock)
		TimerTextBlock->SetText(FText::AsNumber(RemainingTime));

	if (RemainingTime == 0)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		EndMiniGame();
	}
}

void AGMUltraJump_Game::EndMiniGame()
{
	if (bMiniGameEnded) return;
	bMiniGameEnded = true;

	APawn* WinnerPawn = nullptr;
	float  MaxHeight = -FLT_MAX;

	for (APawn* P : AllPlayers)
	{
		if (!IsValid(P)) continue;

		const float* SpawnZ = SpawnZMap.Find(P);
		const float  BaseZ = SpawnZ ? *SpawnZ : P->GetActorLocation().Z;
		const float  HeightMeters = (P->GetActorLocation().Z - BaseZ) * 0.01f;

		if (HeightMeters > MaxHeight)
		{
			MaxHeight = HeightMeters;
			WinnerPawn = P;
		}
	}

	FName WinnerID = NAME_None;
	if (IsValid(WinnerPawn))
	{
		for (const FName& T : WinnerPawn->Tags)
		{
			if (T.ToString().StartsWith(TEXT("Player_")))
			{
				WinnerID = T;
				break;
			}
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (FNameProperty* NP = CastField<FNameProperty>(GI->GetClass()->FindPropertyByName(TEXT("WinnerID"))))
			NP->SetPropertyValue_InContainer(GI, WinnerID);

		if (FIntProperty* IP = CastField<FIntProperty>(GI->GetClass()->FindPropertyByName(TEXT("Set_WinMoney"))))
			IP->SetPropertyValue_InContainer(GI, 100);
	}

	GetWorldTimerManager().ClearTimer(EndHandle);
	GetWorldTimerManager().SetTimer(
		EndHandle,
		[this]()
		{
			UGameplayStatics::OpenLevel(this, FName("ThirdPersonMap"));
		},
		2.0f, false);
}
