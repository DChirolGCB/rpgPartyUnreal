#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HexCoordinates.h"            // votre struct FHexAxialCoordinates + DistanceTo()
#include "HexPathFinder.generated.h"

class UHexGridManager;

/**
 * Composant A* pour trouver un chemin sur la grille hexagonale.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEMO_API UHexPathFinder : public UActorComponent
{
    GENERATED_BODY()

public:
    UHexPathFinder();

    /** 
     * Retourne la liste des coordonnées (axiales) formant le chemin de Start à Goal.
     * Si aucun chemin, renvoie un TArray vide.
     */
    TArray<FHexAxialCoordinates> FindPath(const FHexAxialCoordinates& Start, const FHexAxialCoordinates& Goal) const;

protected:
    virtual void BeginPlay() override;

private:
    /** Référence vers le gestionnaire de grille */
    UPROPERTY()
    UHexGridManager* GridManager;

    /** Heuristique (distance hexagonale) */
    int32 Heuristic(const FHexAxialCoordinates& A, const FHexAxialCoordinates& B) const;
};
