#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "HexCoordinates.h"
#include "HexGridManager.generated.h"

class AHexTile;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEMO_API UHexGridManager : public USceneComponent
{
    GENERATED_BODY()

public:
    UHexGridManager();

    // Nouvelle méthode d'initialisation
    void InitializeGrid(int32 InRadius, TSubclassOf<AHexTile> InHexTileClass);

    // Génère la grille
    void GenerateGrid();

    // Calcule la position d'une tuile avec collision Z
    FVector ComputeTileSpawnPosition(int32 Q, int32 R, float TileSize, const FVector& CenterLocation);

    // Accès à une tuile par coordonnées
    AActor* GetHexTileAt(const FHexAxialCoordinates& Coords) const;

    // Enregistre une tuile dans la map
    void RegisterHexTile(AActor* Tile, const FHexAxialCoordinates& Coords);

    // Converti Offset -> Axial
    static FHexAxialCoordinates OffsetToAxial(int32 X, int32 Y);

    // Vérifie qu'une coordonnée est valide
    bool IsValidCoordinate(const FHexAxialCoordinates& Coords);

    // Retourne les coordonnées voisines
    TArray<FHexAxialCoordinates> GetNeighbors(const FHexAxialCoordinates& Coords);

    const TMap<FHexAxialCoordinates, TWeakObjectPtr<AActor>>& GetHexTiles() const { return HexTiles; }
    void SetGridManager(UHexGridManager* InGridManager) { GridManager = InGridManager; }

    // Rayon de tuile (taille visuelle)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TileSize = 231.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
    FVector GridOrigin = FVector::ZeroVector;

protected:
    virtual void BeginPlay() override;

private:
    int32 GridRadius = 10; 
    TSubclassOf<AHexTile> HexTileClass;

    TMap<FHexAxialCoordinates, TWeakObjectPtr<AActor>> HexTiles;

    static const TArray<FHexAxialCoordinates> HexDirections;

    UPROPERTY()
    UHexGridManager* GridManager;
};
