#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HexCoordinates.h"
#include "HexPawn.generated.h"

// Forward declarations UNIQUEMENT ici (UHT-friendly)
class USpringArmComponent;
class UCameraComponent;
class AHexTile;
class UHexGridManager;

UCLASS()
class DEMO_API AHexPawn : public APawn
{
    GENERATED_BODY()

public:
    AHexPawn();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // --- API move ---
    UFUNCTION(BlueprintCallable, Category="Hex|Move")
    void StartPathFollowing(const TArray<FHexAxialCoordinates>& InPath, class UHexGridManager* InGridManager);

    UFUNCTION(BlueprintCallable, Category="Hex|Move")
    void SetCurrentTile(AHexTile* NewTile);

    UFUNCTION(BlueprintPure, Category="Hex|Move")
    AHexTile* GetCurrentTile() const { return CurrentTile; }

    UFUNCTION(BlueprintCallable, Category="Hex|Move")
    void InitializePawnStartTile(const FHexAxialCoordinates& StartCoords);

public:
    // --- Camera tuning ---
    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="100.0"))
    float CameraHeight = 1500.f;                // longueur du boom

    // Pas de rotation pendant le déplacement
    UPROPERTY(EditAnywhere, Category="Hex|Move")
    bool bFaceDirection = false;

    // Tuning déplacement
    UPROPERTY(EditAnywhere, Category="Hex|Move", meta=(ClampMin="0.05"))
    float StepDuration = 0.2f;

    UPROPERTY(EditAnywhere, Category="Hex|Move")
    bool bEaseInOut = true;

    UPROPERTY(EditAnywhere, Category="Hex|Move", meta=(ClampMin="0.0"))
    float TurnRateDegPerSec = 720.f; // ignoré si bFaceDirection=false

private:
    // Components (fwd-decl + UPROPERTY propre)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
    USpringArmComponent* CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
    UCameraComponent*    TopDownCamera = nullptr;

    // Réfs/état
    UPROPERTY()
    UHexGridManager* GridRef = nullptr;

    UPROPERTY()
    TArray<FHexAxialCoordinates> CurrentPath;

    int32 CurrentStepIndex = 0;
    bool  bIsMoving = false;
    float StepElapsed  = 0.f;

    FVector StartLocation;
    FVector TargetLocation;

    UPROPERTY()
    AHexTile* CurrentTile = nullptr;
};
