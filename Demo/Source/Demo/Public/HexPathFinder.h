#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HexCoordinates.h"
#include "HexPathFinder.generated.h"

class UHexGridManager;

/**
 * A* sur coordonnées axiales standard.
 * Ne considère comme voisins que les 6 directions axiales ET uniquement les tuiles réellement présentes.
 */
UCLASS(ClassGroup=(Hex), meta=(BlueprintSpawnableComponent))
class DEMO_API UHexPathFinder : public UActorComponent
{
    GENERATED_BODY()

public:
    UHexPathFinder();

    /** Trouve un chemin Start -> Goal (incluant les deux) ; vide si impossible. */
    UFUNCTION(BlueprintCallable, Category="Hex|Path")
    TArray<FHexAxialCoordinates> FindPath(const FHexAxialCoordinates& Start,
                                          const FHexAxialCoordinates& Goal);

private:
    /** Renvoie les voisins axiaux standard existants dans la grille. */
    void GetValidNeighbors(UHexGridManager* Grid,
                           const FHexAxialCoordinates& From,
                           TArray<FHexAxialCoordinates>& OutNeighbors) const;

    /** Distance hex (axial) */
    static int32 HexDistance(const FHexAxialCoordinates& A, const FHexAxialCoordinates& B);

    /** Remonte le chemin depuis Goal via Parent */
    static void ReconstructPath(const TMap<FHexAxialCoordinates, FHexAxialCoordinates>& Parent,
                                const FHexAxialCoordinates& Start,
                                const FHexAxialCoordinates& Goal,
                                TArray<FHexAxialCoordinates>& OutPath);
};
