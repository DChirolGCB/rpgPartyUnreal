// HexAnimationManager.h - Add at the top after includes
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexAnimationManager.generated.h"

// Forward declaration
class AHexPawn;

// HexAnimationManager.h - Manages all player animations efficiently
UCLASS()
class DEMO_API AHexAnimationManager : public AActor
{
    GENERATED_BODY()

public:
    AHexAnimationManager();

    // Player registry
    void RegisterPlayer(AHexPawn *Pawn);
    void UnregisterPlayer(AHexPawn *Pawn);

    // Batch updates for performance
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void BatchUpdateAnimations();

    // Animation pooling for multiple players
    UPROPERTY(EditAnywhere, Category = "Animation")
    TMap<FString, class UPaperFlipbook *> SharedAnimationPool;

protected:
    // Separate lists for optimization
    UPROPERTY()
    TArray<TWeakObjectPtr<AHexPawn>> LocalPlayers;

    UPROPERTY()
    TArray<TWeakObjectPtr<AHexPawn>> RemotePlayers;

    // Update frequencies
    UPROPERTY(EditAnywhere, Category = "Performance")
    float LocalPlayerUpdateRate = 0.016f; // 60 FPS

    UPROPERTY(EditAnywhere, Category = "Performance")
    float RemotePlayerUpdateRate = 0.033f; // 30 FPS

private:
    float LocalUpdateAccumulator = 0.0f;
    float RemoteUpdateAccumulator = 0.0f;
};