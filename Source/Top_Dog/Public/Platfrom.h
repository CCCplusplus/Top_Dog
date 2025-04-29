#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Platfrom.generated.h"    // ¡coincide con el nombre del fichero!

UCLASS()
class TOP_DOG_API APlatfrom : public AActor
{
    GENERATED_BODY()

public:
    APlatfrom();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision")
    UBoxComponent* TriggerBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision")
    UStaticMeshComponent* PlatformMesh;


    UPROPERTY(EditDefaultsOnly, Category="Spawning")
    TSubclassOf<APlatfrom> NextPlatformClass;

    UPROPERTY(EditAnywhere, Category="Spawning")
    TArray<float> NextYs;

    UFUNCTION()
    void OnTriggerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                               bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnTriggerOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


    void SpawnNextPlatform();

    bool bHasSpawnedNext = false;


    // xoroshiro128+ para 50/50
    static uint64_t s_state0;
	static uint64_t s_state1;
    static FORCEINLINE uint64_t RotateLeft(uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }
    static uint64_t NextXoroshiro();
};
