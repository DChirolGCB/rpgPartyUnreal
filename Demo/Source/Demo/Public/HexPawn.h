// HexPawn.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HexCoordinates.h"
#include "HexAnimationTypes.h"
#include "HexPawn.generated.h"

// Forward declarations
class USpringArmComponent;
class UCameraComponent;
class UHexSpriteComponent;
class AHexTile;
class UHexGridManager;

/**
 * Pawn that moves tile-to-tile on a hex grid and displays a Paper2D flipbook.
 * Server drives movement. Clients mirror sprite orientation locally.
 */
UCLASS()
class DEMO_API AHexPawn : public APawn
{
    GENERATED_BODY()

public:
    AHexPawn();

    /** Engine lifecycle */
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Start following a path of axial coordinates on a given grid */
    UFUNCTION(BlueprintCallable, Category="Hex|Move")
    void StartPathFollowing(const TArray<FHexAxialCoordinates>& InPath, UHexGridManager* InGridManager);

    /** Set current tile reference (no teleport unless done by caller) */
    UFUNCTION(BlueprintCallable, Category="Hex|Move")
    void SetCurrentTile(AHexTile* NewTile);

    /** Get current tile */
    UFUNCTION(BlueprintPure, Category="Hex|Move")
    AHexTile* GetCurrentTile() const { return CurrentTile; }

    /** Snap pawn to a start tile by axial coordinates */
    UFUNCTION(BlueprintCallable, Category="Hex|Move")
    void InitializePawnStartTile(const FHexAxialCoordinates& StartCoords);

    /** Camera tuning */
    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="100.0"))
    float CameraHeight = 1500.f;

    /** Rotate toward movement direction (disabled by default) */
    UPROPERTY(EditAnywhere, Category="Hex|Move")
    bool bFaceDirection = false;

    /** Time to traverse one step between adjacent tiles */
    UPROPERTY(EditAnywhere, Category="Hex|Move", meta=(ClampMin="0.05"))
    float StepDuration = 0.2f;

    /** Smoothstep easing for movement */
    UPROPERTY(EditAnywhere, Category="Hex|Move")
    bool bEaseInOut = true;

    /** Yaw turn rate when bFaceDirection = true */
    UPROPERTY(EditAnywhere, Category="Hex|Move", meta=(ClampMin="0.0"))
    float TurnRateDegPerSec = 720.f;

    /** Replication setup */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Path replicated from server to clients */
    UPROPERTY(ReplicatedUsing=OnRep_CurrentPath)
    TArray<FHexAxialCoordinates> ReplicatedPath;

    /** Client hook when ReplicatedPath updates */
    UFUNCTION()
    void OnRep_CurrentPath();

    /** Server RPC to request movement */
    UFUNCTION(Server, Reliable)
    void ServerRequestMove(const TArray<FHexAxialCoordinates>& NewPath);

    /** Optional network smoothing (reserved for future use) */
    UPROPERTY(EditAnywhere, Category="Network")
    bool bUseSmoothingForRemotePlayers = true;

    UPROPERTY(EditAnywhere, Category="Network", meta=(ClampMin="0.0", ClampMax="1.0"))
    float NetworkSmoothingRate = 0.1f;

    /** Paper2D sprite component wrapper */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Visual")
    UHexSpriteComponent* SpriteComp = nullptr;

protected:
    /** Local/remote hooks (no-op for now) */
    void TickLocalPlayer(float /*DeltaTime*/) {}
    void TickRemotePlayer(float /*DeltaTime*/) {}

    /** Possession hooks keep camera locked on pawn */
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_Controller() override;

private:
    /** Camera rig */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
    USpringArmComponent* CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
    UCameraComponent* TopDownCamera = nullptr;

    /** Grid and movement state */
    UPROPERTY()
    UHexGridManager* GridRef = nullptr;

    UPROPERTY()
    TArray<FHexAxialCoordinates> CurrentPath;

    int32  CurrentStepIndex = 0;
    bool   bIsMoving = false;
    float  StepElapsed = 0.f;

    FVector StartLocation;
    FVector TargetLocation;

    UPROPERTY()
    AHexTile* CurrentTile = nullptr;

    FVector LastReplicatedLocation;
    FVector SmoothLocation;

    /** Sprite mirroring state */
    UPROPERTY()
    FVector SpriteBaseScale = FVector(1,1,1);

    /** Flip the sprite horizontally relative to camera right vs. move direction */
    void UpdateSpriteMirrorToward(const FVector& From, const FVector& To);
};
