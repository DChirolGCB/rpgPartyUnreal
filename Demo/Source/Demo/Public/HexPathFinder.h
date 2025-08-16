#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HexCoordinates.h"
#include "HexPathFinder.generated.h"

class UHexGridManager;

/** A* sur grille hex (doubled-q), voisins = tuiles réellement présentes. */
UCLASS(ClassGroup=(Hex), meta=(BlueprintSpawnableComponent))
class DEMO_API UHexPathFinder : public UActorComponent
{
	GENERATED_BODY()

public:
	UHexPathFinder();

	/** À appeler au BeginPlay du GM : PathFinder->Init(GridManager); */
	UFUNCTION(BlueprintCallable, Category="Hex|Path")
	void Init(UHexGridManager* InGrid) { GridRef = InGrid; }

	/** Trouve un chemin Start->Goal (Start et Goal inclus). Vide si impossible. */
	UFUNCTION(BlueprintCallable, Category="Hex|Path")
	TArray<FHexAxialCoordinates> FindPath(const FHexAxialCoordinates& Start,
	                                      const FHexAxialCoordinates& Goal);

private:
	UPROPERTY() UHexGridManager* GridRef = nullptr;

	void GetValidNeighbors(const FHexAxialCoordinates& From,
	                       TArray<FHexAxialCoordinates>& Out) const;

	int32 Heuristic(const FHexAxialCoordinates& A, const FHexAxialCoordinates& B) const;

	static void ReconstructPath(const TMap<FHexAxialCoordinates,FHexAxialCoordinates>& Parent,
	                            const FHexAxialCoordinates& Start,
	                            const FHexAxialCoordinates& Goal,
	                            TArray<FHexAxialCoordinates>& OutPath);
};
