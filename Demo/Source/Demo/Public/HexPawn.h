#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HexCoordinates.h"
#include "HexAnimationTypes.h"
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
    UFUNCTION(BlueprintCallable, Category = "Hex|Move")
    void StartPathFollowing(const TArray<FHexAxialCoordinates> &InPath, class UHexGridManager *InGridManager);

    UFUNCTION(BlueprintCallable, Category = "Hex|Move")
    void SetCurrentTile(AHexTile *NewTile);

    UFUNCTION(BlueprintPure, Category = "Hex|Move")
    AHexTile *GetCurrentTile() const { return CurrentTile; }

    UFUNCTION(BlueprintCallable, Category = "Hex|Move")
    void InitializePawnStartTile(const FHexAxialCoordinates &StartCoords);

    // --- Camera tuning ---
    UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "100.0"))
    float CameraHeight = 1500.f; // longueur du boom

    // Pas de rotation pendant le déplacement
    UPROPERTY(EditAnywhere, Category = "Hex|Move")
    bool bFaceDirection = false;

    // Tuning déplacement
    UPROPERTY(EditAnywhere, Category = "Hex|Move", meta = (ClampMin = "0.05"))
    float StepDuration = 0.2f;

    UPROPERTY(EditAnywhere, Category = "Hex|Move")
    bool bEaseInOut = true;

    UPROPERTY(EditAnywhere, Category = "Hex|Move", meta = (ClampMin = "0.0"))
    float TurnRateDegPerSec = 720.f; // ignoré si bFaceDirection=false

    // Network setup
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // Movement replication
    UPROPERTY(ReplicatedUsing = OnRep_CurrentPath)
    TArray<FHexAxialCoordinates> ReplicatedPath;

    UFUNCTION()
    void OnRep_CurrentPath();

    // Server-authoritative movement
    UFUNCTION(Server, Reliable)
    void ServerRequestMove(const TArray<FHexAxialCoordinates> &NewPath);

    // Smooth interpolation for remote players
    UPROPERTY(EditAnywhere, Category = "Network")
    bool bUseSmoothingForRemotePlayers = true;

    UPROPERTY(EditAnywhere, Category = "Network", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float NetworkSmoothingRate = 0.1f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Visual")
    class UHexSpriteComponent* SpriteComp;

protected:
    void TickLocalPlayer(float DeltaTime);
    void TickRemotePlayer(float DeltaTime);
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_Controller() override;

private:
    // Components (fwd-decl + UPROPERTY propre)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent *CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent *TopDownCamera = nullptr;

    // Réfs/état
    UPROPERTY()
    UHexGridManager *GridRef = nullptr;

    UPROPERTY()
    TArray<FHexAxialCoordinates> CurrentPath;

    int32 CurrentStepIndex = 0;
    bool bIsMoving = false;
    float StepElapsed = 0.f;

    FVector StartLocation;
    FVector TargetLocation;

    UPROPERTY()
    AHexTile *CurrentTile = nullptr;

    FVector LastReplicatedLocation;
    FVector SmoothLocation;
};
