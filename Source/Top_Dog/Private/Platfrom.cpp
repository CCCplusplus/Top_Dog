#include "Platfrom.h"
#include "Ultra_Jump_Boy.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

uint64_t APlatfrom::s_state0 = FDateTime::Now().GetTicks();
uint64_t APlatfrom::s_state1 = s_state0 ^ 0x9E3779B97F4A7C15ULL;

APlatfrom::APlatfrom()
{
    PrimaryActorTick.bCanEverTick = false;


    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;
    PlatformMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PlatformMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    PlatformMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);


    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootComponent);
    TriggerBox->SetRelativeLocation(FVector(0, 0, 150.f));
    TriggerBox->InitBoxExtent(FVector(200.f, 200.f, 50.f));
    TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APlatfrom::OnTriggerOverlapBegin);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APlatfrom::OnTriggerOverlapEnd);

    NextYs = { 126.7512f, -167.904128f };
}

void APlatfrom::BeginPlay()
{
    Super::BeginPlay();
}

void APlatfrom::Tick(float DeltaTime)
{
}

void APlatfrom::OnTriggerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (AUltra_Jump_Boy* Player = Cast<AUltra_Jump_Boy>(OtherActor))
    {
        float PlatZ = GetActorLocation().Z;
        float PlyZ = Player->GetActorLocation().Z;

        if (PlyZ < PlatZ)
            PlatformMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        else
        {
            PlatformMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

            if (!bHasSpawnedNext)
            {
                bHasSpawnedNext = true;
                SpawnNextPlatform();
            }
        }
    }
}

void APlatfrom::OnTriggerOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AUltra_Jump_Boy* Player = Cast<AUltra_Jump_Boy>(OtherActor))
        PlatformMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void APlatfrom::SpawnNextPlatform()
{
    if (!NextPlatformClass) return;

    FVector Loc = GetActorLocation();
    float NextZ = Loc.Z + 250.f;
    uint64_t rnd = NextXoroshiro();
    int idx = rnd & 1;
    float NextY = NextYs.IsValidIndex(idx) ? NextYs[idx] : NextYs[0];

    FVector SpawnLoc(Loc.X, NextY, NextZ);
    GetWorld()->SpawnActor<APlatfrom>(NextPlatformClass, SpawnLoc, FRotator::ZeroRotator);
}

uint64_t APlatfrom::NextXoroshiro()
{
    uint64_t s0 = s_state0;
    uint64_t s1 = s_state1;
    uint64_t result = s0 + s1;

    s1 ^= s0;
    s_state0 = RotateLeft(s0, 55) ^ s1 ^ (s1 << 14);
    s_state1 = RotateLeft(s1, 36);
    return result;
}
