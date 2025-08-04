#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HexCoordinates.h"
#include "HexPathfinder.generated.h"

UCLASS(BlueprintType)
class DEMO_API UHexPathfinder : public UObject
{
    GENERATED_BODY()

public:
    UHexPathfinder();

private:
    UPROPERTY()
    TWeakObjectPtr<class UHexGridManager> GridManager;

public:
    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
    void Initialize(UHexGridManager* InGridManager);
    
    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
    TArray<FHexAxialCoordinates> FindPath(const FHexAxialCoordinates& Start, const FHexAxialCoordinates& Goal);

private:
    float CalculateHeuristic(const FHexAxialCoordinates& Start, const FHexAxialCoordinates& Goal) const;
    TArray<FHexAxialCoordinates> ReconstructPath(const FHexAxialCoordinates& Goal, const TMap<FHexAxialCoordinates, FHexAxialCoordinates>& CameFrom);
};