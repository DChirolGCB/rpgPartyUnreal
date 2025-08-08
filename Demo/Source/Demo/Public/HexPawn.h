#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HexCoordinates.h"
#include "Components/TimelineComponent.h"
#include "HexPawn.generated.h"

class UTimelineComponent;
class UCurveFloat;
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

    void StartPathFollowing(const TArray<FHexAxialCoordinates>& InPath);

    AHexTile* GetCurrentTile() const { return CurrentTile; }

	UFUNCTION(BlueprintCallable)
    void InitializePawnStartTile(const FHexAxialCoordinates& StartCoords);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetCurrentTile(AHexTile* NewTile);

protected:
    UPROPERTY(EditAnywhere, Category="Movement")
    UCurveFloat* MovementCurve;

private:
    void MoveToNextStep();
    void OnMovementUpdate(float Alpha);
    void OnMovementComplete();

    UPROPERTY()
    TArray<FHexAxialCoordinates> PathToFollow;

    int32 CurrentStepIndex = 0;

    FVector StartLocation;
    FVector TargetLocation;

    UPROPERTY()
    AHexTile* CurrentTile;

    UPROPERTY()
    UTimelineComponent* MovementTimeline;

    FOnTimelineFloat UpdateFunction;
    FOnTimelineEvent FinishedFunction;
};
