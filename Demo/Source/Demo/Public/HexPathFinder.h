// HexPathFinder.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HexCoordinates.h"
#include "HexPathFinder.generated.h"

class UHexGridManager;

/**
 * A* pathfinder over a hex grid (doubled-q axial).
 * Neighbors come from the grid and only include existing tiles.
 */
UCLASS(ClassGroup=(Hex), meta=(BlueprintSpawnableComponent))
class DEMO_API UHexPathFinder : public UActorComponent
{
	GENERATED_BODY()

public:
	UHexPathFinder();

	/** Bind the pathfinder to a grid instance (call from GameMode BeginPlay). */
	UFUNCTION(BlueprintCallable, Category="Hex|Path")
	void Init(UHexGridManager* InGrid) { GridRef = InGrid; }

	/**
	 * Compute a path from Start to Goal (both included).
	 * Returns empty array if no path is found or grid is missing.
	 */
	UFUNCTION(BlueprintCallable, Category="Hex|Path")
	TArray<FHexAxialCoordinates> FindPath(const FHexAxialCoordinates& Start,
	                                      const FHexAxialCoordinates& Goal);

private:
	/** Grid provider for neighbors and distance. */
	UPROPERTY()
	UHexGridManager* GridRef = nullptr;

	/** Fetch valid neighbors for a given axial coordinate. */
	void GetValidNeighbors(const FHexAxialCoordinates& From,
	                       TArray<FHexAxialCoordinates>& Out) const;

	/** Admissible heuristic based on axial distance. */
	int32 Heuristic(const FHexAxialCoordinates& A,
	                const FHexAxialCoordinates& B) const;

	/** Build final path by walking parents from Goal to Start. */
	static void ReconstructPath(const TMap<FHexAxialCoordinates,FHexAxialCoordinates>& Parent,
	                            const FHexAxialCoordinates& Start,
	                            const FHexAxialCoordinates& Goal,
	                            TArray<FHexAxialCoordinates>& OutPath);
};
