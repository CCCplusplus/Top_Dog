#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Camera/CameraActor.h"
#include "ColorCoded.generated.h"

class ALunker;

UCLASS()
class TOP_DOG_API AColorCoded : public AGameModeBase
{
	GENERATED_BODY()

public:
	AColorCoded();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------- Config ----------
public:
	UPROPERTY(EditDefaultsOnly, Category = "Players")
	TSubclassOf<ALunker> LunkerClass;

	UPROPERTY(EditAnywhere, Category = "Round")
	float SinkDeltaZ = 650.f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float SinkTimeStart = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float SinkTimeMin = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float SinkTimeStep = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float RiseTimeStart = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float RiseTimeMin = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float RiseTimeStep = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float HoldTime = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float BetweenRoundsDelay = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float FirstSelectionDelay = 2.0f;

	UPROPERTY(EditAnywhere, Category = "World")
	float WaterSurfaceZ = 0.f;

	UPROPERTY(EditAnywhere, Category = "World")
	float KillZBelowSurface = 80.f;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* RoundStartSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Return")
	FName BoardLevelName = FName("ThirdPersonMap");

protected:

	static int32 ReadNumPlayersFromGICC(const UWorld* World);
	static int32 GetLaneFromStartCC(const AActor* A); // tags: Start_1..4


	static uint64 s0;
	static uint64 s1;
	static FORCEINLINE uint64 RotL(uint64 x, int k) { return (x << k) | (x >> (64 - k)); }
	static uint64 NextXoroshiro();


	UEnum* PlatformEnum = nullptr;
	int32  EnumCount = 0;
	uint8  CurrentType = 0;

	bool   ResolvePlatformEnum();
	uint8  RandomType();
	bool   ReadEnumOnActor(AActor* A, const TCHAR* PropA, const TCHAR* PropB, uint8& OutVal) const;
	void   WriteEnumOnActor(AActor* A, const TCHAR* PropA, const TCHAR* PropB, uint8 Value) const;

	ACameraActor* MainCam = nullptr;

	struct FPlatInfo
	{
		AActor* Actor = nullptr;
		FVector BaseLoc = FVector::ZeroVector;
		uint8   TypeVal = 0;
	};
	TArray<FPlatInfo> Platforms;
	TArray<AActor*>   Billboards;

	TArray<ALunker*> AllPlayers;


	enum class EPhase : uint8 { Idle, PreLower, Lowering, Hold, Raising, InterRound };
	EPhase Phase = EPhase::Idle;

	bool bFirstRound = true;

	float SinkTime = 0.f;
	float RiseTime = 0.f;
	float PhaseTime = 0.f;
	float PhaseDur = 0.f;


	void CollectSceneActors();
	void SpawnAllPlayers();
	void SetupSingleCamera();
	void EnforceSharedCamera();
	void StartRound();
	void SetBillboardsTo(uint8 Type);
	void EnterPhase(EPhase NewPhase, float Duration);
	void TickPhase(float DT);
	void CheckLocksOnWaterSurface();
	bool AllEliminatedCleared() const;
	void ApplyPlatformPositions(float AlphaDown);
	void CheckEliminationsOnHold();
	void AfterRoundAdjustTimes();
	void CheckWinConditions();
	void EndMiniGame(ALunker* Winner);


	bool IsLoserPlatform(const FPlatInfo& P) const { return P.TypeVal != CurrentType; }
	bool IsAlive(const ALunker* P) const;
};
