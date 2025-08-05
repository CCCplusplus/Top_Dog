#include "AGMChicken_Game.h"
#include "ChickenMan.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"       
#include "Engine/World.h"
#include <Algo/AllOf.h>

static int32 ReadNumPlayersFromGI(const UWorld* World)
{
    if (!World) return 1;
    if (const UGameInstance* GI = World->GetGameInstance())
    {
        static const FName VarName(TEXT("NumberofPlayer"));
        if (const FIntProperty* P =
            CastField<FIntProperty>(GI->GetClass()->FindPropertyByName(VarName)))
            return P->GetPropertyValue_InContainer(GI);
    }
    return 1;
}

static int32 GetLaneFromStart(const AActor* A)
{
    if (!A) return INDEX_NONE;
    for (const FName& Tag : A->Tags)
    {
        const FString S = Tag.ToString();
        if (S.StartsWith(TEXT("PStart_")))
        {
            const int32 N = FCString::Atoi(*S.Mid(7));
            if (N >= 1 && N <= 4) return N - 1;
        }
    }
    return INDEX_NONE;
}

static int32 GetLaneFromCrusher(const AActor* A)
{
    if (!A) return INDEX_NONE;
    for (const FName& Tag : A->Tags)
    {
        const FString S = Tag.ToString();
        if (S.StartsWith(TEXT("Crusher_")))
        {
            const int32 N = FCString::Atoi(*S.Mid(8));
            if (N >= 1 && N <= 4) return N - 1;
        }
    }
    return INDEX_NONE;
}

void AAGMChicken_Game::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();
    if (!World) return;

    FTransform LaneTransforms[4];
    bool       bLaneFound[4] = { false,false,false,false };

    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        const int32 Lane = GetLaneFromStart(*It);
        if (Lane != INDEX_NONE && Lane < 4)
        {
            LaneTransforms[Lane] = It->GetActorTransform();
            bLaneFound[Lane] = true;
        }
    }
    for (int32 i = 0; i < 4; ++i)
        if (!bLaneFound[i])
            LaneTransforms[i] = FTransform(FVector(i * 300.f, 0.f, 300.f));

    AActor* LaneCrushers[4] = { nullptr,nullptr,nullptr,nullptr };
    {
        TArray<AActor*> Found;
        UGameplayStatics::GetAllActorsWithTag(World, TEXT("Crusher_1"), Found);
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        const int32 Lane = GetLaneFromCrusher(*It);
        if (Lane != INDEX_NONE && Lane < 4)
            LaneCrushers[Lane] = *It;
        
    }

    const int32 NumHumans = FMath::Clamp(ReadNumPlayersFromGI(World), 1, 4);


    AllPlayers.Empty();

    for (int32 i = 0; i < 4; ++i)
    {
        APlayerController* PC = nullptr;

        if (i == 0)
        {
            PC = UGameplayStatics::GetPlayerController(World, 0);
            if (!PC)
                PC = UGameplayStatics::CreatePlayer(World, 0, true);

        }
        else
        {
            PC = UGameplayStatics::GetPlayerController(World, i);
            if (!PC)
                PC = UGameplayStatics::CreatePlayer(World, i, true);
        }

        if (!PC) continue;


        if (APawn* OldPawn = PC->GetPawn())
            OldPawn->Destroy();
        


        AChickenMan* Pawn = World->SpawnActor<AChickenMan>(ChickenManClass,
            LaneTransforms[i]);
        if (!Pawn) continue;

        PC->Possess(Pawn);
        PC->SetViewTarget(Pawn);


        Pawn->PlayerID = FName(*FString::Printf(TEXT("Player_%d"), i + 1));

        Pawn->Tags.Add(Pawn->PlayerID);

        const bool bIsBot = (i >= NumHumans);
        Pawn->bIsBot = bIsBot;
        if (bIsBot)
            PC->DisableInput(PC);



        if (LaneCrushers[i])
        {
            Pawn->CrusherActor = LaneCrushers[i];

            if (APawn* CrusherPawn = Cast<APawn>(LaneCrushers[i]))
            {
                if (AAIController* AIC = Cast<AAIController>(CrusherPawn->GetController()))
                {
                    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
                    if (BB)
                        BB->SetValueAsObject(TEXT("Player"), Pawn);
                    
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Lane %d: No Crusher found!"), i + 1);
        }

        AllPlayers.Add(Pawn);
    }
}


void AAGMChicken_Game::OnPlayerEliminated(AChickenMan*)
{
    const bool bDone = Algo::AllOf(AllPlayers, [](AChickenMan* P)
        {
            return !IsValid(P) || P->bEliminated || P->hasturned;
        });

    if (bDone)
        EndMiniGame();

}


void AAGMChicken_Game::EndMiniGame()
{
    AChickenMan* Winner = nullptr;
    float        MaxDist = -FLT_MAX;

    for (AChickenMan* P : AllPlayers)
    {
        if (!IsValid(P) || P->bEliminated) continue;
        if (P->CurrentDistanceMeters > 0.f && P->CurrentDistanceMeters > MaxDist)
        {
            MaxDist = P->CurrentDistanceMeters;
            Winner = P;
        }
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (FNameProperty* NP =
            CastField<FNameProperty>(GI->GetClass()->FindPropertyByName(TEXT("WinnerID"))))
        {
            NP->SetPropertyValue_InContainer(GI, Winner ? Winner->PlayerID : NAME_None);
        }
        if (FIntProperty* IP =
            CastField<FIntProperty>(GI->GetClass()->FindPropertyByName(TEXT("Set_WinMoney"))))
        {
            IP->SetPropertyValue_InContainer(GI, 100);
        }
    }

    FTimerHandle DelayHandle;
    GetWorldTimerManager().SetTimer(DelayHandle, [this]()
        {
            UGameplayStatics::OpenLevel(this, FName("ThirdPersonMap"));
        }, 2.0f, false);
}
