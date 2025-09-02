#include "ColorCoded.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"
#include "Lunker.h"
#include "Engine/GameViewportClient.h"

// ---------- RNG seeds ----------
uint64 AColorCoded::s0 = FDateTime::Now().GetTicks();
uint64 AColorCoded::s1 = AColorCoded::s0 ^ 0x9E3779B97F4A7C15ULL;

// ---------- Helpers GI / PlayerStart ----------
int32 AColorCoded::ReadNumPlayersFromGICC(const UWorld* World)
{
	if (!World) return 1;
	if (const UGameInstance* GI = World->GetGameInstance())
	{
		static const FName Var(TEXT("NumberofPlayer"));
		if (const FIntProperty* P = CastField<FIntProperty>(GI->GetClass()->FindPropertyByName(Var)))
			return FMath::Clamp(P->GetPropertyValue_InContainer(GI), 1, 4);
	}
	return 1;
}

int32 AColorCoded::GetLaneFromStartCC(const AActor* A)
{
	if (!A) return INDEX_NONE;
	for (const FName& Tag : A->Tags)
	{
		const FString S = Tag.ToString(); // Start_1..4
		if (S.StartsWith(TEXT("Start_")))
		{
			const int32 N = FCString::Atoi(*S.Mid(6));
			if (N >= 1 && N <= 4) return N - 1;
		}
	}
	return INDEX_NONE;
}

// ---------- RNG ----------
uint64 AColorCoded::NextXoroshiro()
{
	uint64 r = s0 + s1;
	s1 ^= s0;
	s0 = RotL(s0, 55) ^ s1 ^ (s1 << 14);
	s1 = RotL(s1, 36);
	return r;
}

// ---------- Enum helpers ----------
bool AColorCoded::ResolvePlatformEnum()
{
	if (PlatformEnum) return true;
	PlatformEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("E_PlatformType"));
	if (!PlatformEnum) return false;
	EnumCount = 0;
	for (int32 i = 0; i < PlatformEnum->NumEnums(); ++i)
	{
		const FString Name = PlatformEnum->GetNameStringByIndex(i);
		if (!Name.Contains(TEXT("MAX")) && !Name.Contains(TEXT("MAX_")))
			EnumCount++;
	}
	return EnumCount > 0;
}

uint8 AColorCoded::RandomType()
{
	if (!ResolvePlatformEnum()) return 0;
	const uint32 r = (uint32)(NextXoroshiro() & 0xffffffffu);
	return (uint8)(r % (uint32)EnumCount);
}

bool AColorCoded::ReadEnumOnActor(AActor* A, const TCHAR* PropA, const TCHAR* PropB, uint8& OutVal) const
{
	if (!A) return false;
	if (FByteProperty* BP = CastField<FByteProperty>(A->GetClass()->FindPropertyByName(PropA)))
	{
		OutVal = BP->GetPropertyValue_InContainer(A); return true;
	}
	if (FByteProperty* BP2 = CastField<FByteProperty>(A->GetClass()->FindPropertyByName(PropB)))
	{
		OutVal = BP2->GetPropertyValue_InContainer(A); return true;
	}
	if (FEnumProperty* EP = CastField<FEnumProperty>(A->GetClass()->FindPropertyByName(PropA)))
	{
		OutVal = (uint8)EP->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(EP->ContainerPtrToValuePtr<void>(A)); return true;
	}
	if (FEnumProperty* EP2 = CastField<FEnumProperty>(A->GetClass()->FindPropertyByName(PropB)))
	{
		OutVal = (uint8)EP2->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(EP2->ContainerPtrToValuePtr<void>(A)); return true;
	}
	return false;
}

void AColorCoded::WriteEnumOnActor(AActor* A, const TCHAR* PropA, const TCHAR* PropB, uint8 Value) const
{
	if (!A) return;
	if (FByteProperty* BP = CastField<FByteProperty>(A->GetClass()->FindPropertyByName(PropA)))
	{
		BP->SetPropertyValue_InContainer(A, Value); return;
	}
	if (FByteProperty* BP2 = CastField<FByteProperty>(A->GetClass()->FindPropertyByName(PropB)))
	{
		BP2->SetPropertyValue_InContainer(A, Value); return;
	}
	if (FEnumProperty* EP = CastField<FEnumProperty>(A->GetClass()->FindPropertyByName(PropA)))
	{
		EP->GetUnderlyingProperty()->SetIntPropertyValue(EP->ContainerPtrToValuePtr<void>(A), (int64)Value); return;
	}
	if (FEnumProperty* EP2 = CastField<FEnumProperty>(A->GetClass()->FindPropertyByName(PropB)))
	{
		EP2->GetUnderlyingProperty()->SetIntPropertyValue(EP2->ContainerPtrToValuePtr<void>(A), (int64)Value); return;
	}
}

// ---------- ctor ----------
AColorCoded::AColorCoded()
{
	PrimaryActorTick.bCanEverTick = true;
}

// ---------- BeginPlay ----------
void AColorCoded::BeginPlay()
{
	Super::BeginPlay();

	CollectSceneActors();
	SpawnAllPlayers();
	SetupSingleCamera();

	SinkTime = SinkTimeStart;
	RiseTime = RiseTimeStart;

	EnterPhase(EPhase::InterRound, 0.2f); // arranque breve
}

void AColorCoded::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EnforceSharedCamera();
	TickPhase(DeltaSeconds);
}


// ---------- World discovery ----------
void AColorCoded::CollectSceneActors()
{
	UWorld* W = GetWorld();
	if (!W) return;

	Platforms.Empty();
	Billboards.Empty();

	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* A = *It;
		const FString CN = A->GetClass()->GetName();

		// Billboards: nombre de clase contiene "Billboard"
		if (CN.Contains(TEXT("Billboard")))
		{
			Billboards.Add(A);
			continue;
		}

		// SeaPlatform (o BP cuyo nombre contiene "SeaPlatform")
		if (CN.Contains(TEXT("SeaPlatform")))
		{
			FPlatInfo Info;
			Info.Actor = A;
			Info.BaseLoc = A->GetActorLocation();

			uint8 Val = 0;
			ReadEnumOnActor(A, TEXT("PlatformType"), TEXT("Type"), Val);
			Info.TypeVal = Val;

			Platforms.Add(Info);
		}

		// Cámara principal (tag "mainCam" si existe; si no, primera CameraActor)
		if (!MainCam)
			if (ACameraActor* Cam = Cast<ACameraActor>(A))
				if (Cam->ActorHasTag(TEXT("mainCam")) || !MainCam)
					MainCam = Cam;
	}

	// fallback cámara si no se etiquetó
	if (!MainCam)
		for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It) { MainCam = *It; break; }
}

void AColorCoded::SpawnAllPlayers()
{
	UWorld* World = GetWorld();
	if (!World || !ensure(LunkerClass)) return;

	FTransform LaneTF[4];
	bool bFound[4] = { false,false,false,false };

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const int32 L = GetLaneFromStartCC(*It);
		if (L != INDEX_NONE)
		{
			LaneTF[L] = It->GetActorTransform();
			bFound[L] = true;
		}
	}
	for (int32 i = 0; i < 4; ++i)
		if (!bFound[i]) LaneTF[i] = FTransform(FVector(i * 200.f, 0.f, 200.f));

	const int32 NumHumans = ReadNumPlayersFromGICC(World);

	AllPlayers.Empty();
	for (int32 i = 0; i < 4; ++i)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, i);
		if (!PC) PC = UGameplayStatics::CreatePlayer(World, i, true);
		if (!PC) continue;

		if (APawn* Old = PC->GetPawn()) Old->Destroy();

		ALunker* P = World->SpawnActor<ALunker>(LunkerClass, LaneTF[i]);
		if (!P) continue;

		PC->Possess(P);
		PC->SetViewTarget(P);

		const bool bHuman = (i < NumHumans);
		if (bHuman) PC->EnableInput(PC); else PC->DisableInput(PC);

		P->bIsBot = !bHuman;

		const FName Tag(*FString::Printf(TEXT("Player_%d"), i + 1));
		P->Tags.AddUnique(Tag);

		AllPlayers.Add(P);
	}
}

void AColorCoded::SetupSingleCamera()
{
	if (UGameViewportClient* VP = GetWorld()->GetGameViewport())
		VP->SetForceDisableSplitscreen(true);
	

	if (MainCam)
	{
		for (int32 i = 0; i < 4; ++i)
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), i))
				PC->SetViewTargetWithBlend(MainCam, 0.f);
	}
}

void AColorCoded::EnforceSharedCamera()
{
	if (UGameViewportClient* VP = GetWorld()->GetGameViewport())
		VP->SetForceDisableSplitscreen(true);

	if (MainCam)
	{
		for (int32 i = 0; i < 4; ++i)
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), i))
			{
				if (PC->GetViewTarget() != MainCam)
					PC->SetViewTargetWithBlend(MainCam, 0.f);

			}
		}
	}
}


void AColorCoded::StartRound()
{
	CurrentType = RandomType();
	SetBillboardsTo(CurrentType);

	if (RoundStartSound)
		UGameplayStatics::PlaySound2D(this, RoundStartSound);
	

	// notifica a los bots hacia dónde moverse
	// target = centro XY de la plataforma segura (primer match)
	FVector Target = FVector::ZeroVector;
	for (const FPlatInfo& P : Platforms)
		if (!IsLoserPlatform(P)) { Target = P.BaseLoc; break; }

	for (ALunker* L : AllPlayers)
	{
		if (IsValid(L) && L->bIsBot && !L->bEliminated)
		{
			const float Delay = FMath::FRandRange(0.2f, 1.5f);
			L->OnNewRound(Target, Delay);
		}
	}

	// Antes: EnterPhase(EPhase::Lowering, SinkTime);
	if (bFirstRound)
	{
		EnterPhase(EPhase::PreLower, FirstSelectionDelay);
		bFirstRound = false;
	}
	else
		EnterPhase(EPhase::Lowering, SinkTime);
	

}

void AColorCoded::SetBillboardsTo(uint8 Type)
{
	static const FName NameA(TEXT("BillboardType"));
	static const FName NameB(TEXT("Type"));
	static const FName NameC(TEXT("Billboard_Type"));
	static const FName FnApply(TEXT("ApplyBillboardType"));

	for (AActor* B : Billboards)
	{
		WriteEnumOnActor(B, *NameA.ToString(), *NameB.ToString(), Type);
		WriteEnumOnActor(B, *NameC.ToString(), *NameB.ToString(), Type);

		if (UFunction* Fn = B->FindFunction(FnApply))
		{
			struct { uint8 NewType; } Params{ Type };
			B->ProcessEvent(Fn, &Params);
		}

		B->MarkComponentsRenderStateDirty();
		B->ForceNetUpdate();

		B->RerunConstructionScripts();
	}
}



void AColorCoded::EnterPhase(EPhase NewPhase, float Duration)
{
	Phase = NewPhase;
	PhaseDur = Duration;
	PhaseTime = 0.f;
}

void AColorCoded::TickPhase(float DT)
{
	switch (Phase)
	{
	case EPhase::InterRound:
		PhaseTime += DT;
		if (PhaseTime >= PhaseDur) StartRound();
		break;

	case EPhase::PreLower:
		PhaseTime += DT;
		CheckLocksOnWaterSurface();
		if (PhaseTime >= PhaseDur) EnterPhase(EPhase::Lowering, SinkTime);
		break;

	case EPhase::Lowering:
		PhaseTime += DT;
		CheckLocksOnWaterSurface();
		ApplyPlatformPositions(FMath::Clamp(PhaseTime / FMath::Max(0.01f, PhaseDur), 0.f, 1.f));
		if (PhaseTime >= PhaseDur) EnterPhase(EPhase::Hold, HoldTime);
		break;

	case EPhase::Hold:
		PhaseTime += DT;
		CheckLocksOnWaterSurface();
		if (PhaseTime > 0.1f) CheckEliminationsOnHold();

		if (PhaseTime >= PhaseDur)
		{
			if (AllEliminatedCleared())
				EnterPhase(EPhase::Raising, RiseTime);
			
		}
		break;

	case EPhase::Raising:
		PhaseTime += DT;
		{
			const float AlphaBack = 1.f - FMath::Clamp(PhaseTime / FMath::Max(0.01f, PhaseDur), 0.f, 1.f);
			ApplyPlatformPositions(AlphaBack);
		}
		if (PhaseTime >= PhaseDur)
		{
			CheckWinConditions();
			AfterRoundAdjustTimes();
			EnterPhase(EPhase::InterRound, BetweenRoundsDelay);
		}
		break;

	default: break;
	}
}

void AColorCoded::CheckLocksOnWaterSurface()
{
	for (ALunker* L : AllPlayers)
	{
		if (!IsAlive(L)) continue;
		const float Z = L->GetActorLocation().Z;
		if (!L->AreInputsLocked() && Z <= WaterSurfaceZ + 1.f) // tolerancia mínima
		{
			L->LockInputs(); // humano: DisableInput, bot: deja de moverse
		}
	}
}

bool AColorCoded::AllEliminatedCleared() const
{
	for (const ALunker* L : AllPlayers)
	{
		if (!L) continue;
		if (!IsAlive(L))
		{
			// Debe tener colisión quitada Y estar ya yéndose hacia abajo
			const bool bNoCollision = L->HasCollisionDisabled();
			const bool bBelowSurface = (L->GetActorLocation().Z <= WaterSurfaceZ - 5.f);
			if (!(bNoCollision && bBelowSurface))
			{
				return false;
			}
		}
	}
	return true;
}


void AColorCoded::ApplyPlatformPositions(float AlphaDown)
{
	for (FPlatInfo& P : Platforms)
	{
		const bool bLoser = IsLoserPlatform(P);
		const float Down = bLoser ? AlphaDown : 0.f;
		const FVector NewLoc = P.BaseLoc + FVector(0, 0, -Down * SinkDeltaZ);
		P.Actor->SetActorLocation(NewLoc, false, nullptr, ETeleportType::None);
	}
}

void AColorCoded::CheckEliminationsOnHold()
{
	const float KillZ = WaterSurfaceZ - KillZBelowSurface;

	for (ALunker* L : AllPlayers)
	{
		if (!IsAlive(L)) continue;

		if (L->GetActorLocation().Z < KillZ)
		{
			L->Eliminate();
		}
	}
}

void AColorCoded::AfterRoundAdjustTimes()
{
	SinkTime = FMath::Max(SinkTime - SinkTimeStep, SinkTimeMin);
	RiseTime = FMath::Max(RiseTime - RiseTimeStep, RiseTimeMin);
}

bool AColorCoded::IsAlive(const ALunker* P) const
{
	return IsValid(P) && !P->bEliminated;
}

void AColorCoded::CheckWinConditions()
{
	int32 AliveCount = 0;
	ALunker* Candidate = nullptr;

	for (ALunker* L : AllPlayers)
	{
		if (IsAlive(L))
		{
			AliveCount++;
			Candidate = L;
		}
	}

	if (AliveCount <= 1)
		EndMiniGame((AliveCount == 1) ? Candidate : nullptr);
}

void AColorCoded::EndMiniGame(ALunker* Winner)
{
	if (!GetWorld() || IsActorBeingDestroyed()) return;

	FName WinnerID = NAME_None;
	if (Winner)
	{
		for (const FName& T : Winner->Tags)
			if (T.ToString().StartsWith(TEXT("Player_"))) { WinnerID = T; break; }
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (FNameProperty* NP = CastField<FNameProperty>(GI->GetClass()->FindPropertyByName(TEXT("WinnerID"))))
			NP->SetPropertyValue_InContainer(GI, WinnerID);

		if (FIntProperty* IP = CastField<FIntProperty>(GI->GetClass()->FindPropertyByName(TEXT("Set_WinMoney"))))
			IP->SetPropertyValue_InContainer(GI, Winner ? 100 : 0);
	}

	for (ALunker* L : AllPlayers)
		if (IsValid(L))
			GetWorldTimerManager().ClearAllTimersForObject(L);
	GetWorldTimerManager().ClearAllTimersForObject(this);

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
		{
			UGameplayStatics::OpenLevel(this, BoardLevelName);
		}, 1.5f, false);

	SetActorTickEnabled(false);
}
