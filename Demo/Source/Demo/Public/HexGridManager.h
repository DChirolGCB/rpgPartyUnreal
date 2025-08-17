
#pragma once
#include "CoreMinimal.h"
#include "HexCoordinates.h"
#include "HexGridManager.generated.h"

class AHexTile;

/**
 * Gère la génération et l'indexation d'une grille hexagonale (axial Q,R).
 * - Placement XY inspiré de l'ancienne version (offset demi-ligne sur parité configurable)
 * - Z déterminé par un line trace vertical (ECC_Visibility + fallback Static/Dynamic)
 * - Stockage des tuiles et requêtes (GetHexTileAt / GetNeighbors)
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEMO_API UHexGridManager : public UActorComponent
{
    GENERATED_BODY()

public:
    // --- ctor ---
    UHexGridManager();

    // --- API ---

    void ForEachTile(TFunctionRef<void(const FHexAxialCoordinates&, AHexTile*)> Fn) const;
    /** Génère la grille (rayon en tuiles, et classe de tuile à instancier) */
    UFUNCTION(BlueprintCallable, Category = "Hex|Generation")
    void InitializeGrid(int32 Radius, TSubclassOf<AHexTile> TileClass);

    /** Accès direct à une tuile (nullptr si absente) */
    UFUNCTION(BlueprintCallable, Category = "Hex|Query")
    AHexTile *GetHexTileAt(const FHexAxialCoordinates &Coords) const;

    /** Renvoie la liste des voisins existants autour d’une coordonnée */
    UFUNCTION(BlueprintCallable, Category = "Hex|Query")
    TArray<FHexAxialCoordinates> GetNeighbors(const FHexAxialCoordinates &Coords) const;

    /** Accès lecture à la map des tuiles (utile pour pathfinding etc.) */
    const TMap<FHexAxialCoordinates, TWeakObjectPtr<AHexTile>>& GetHexTiles() const { return TilesMap; }

    // Cache de voisins calculés en XY (réels)
    TMap<FHexAxialCoordinates, TArray<FHexAxialCoordinates>> WorldNeighbors;

    // Recalcule le cache (à appeler après la génération des tuiles)
    UFUNCTION(BlueprintCallable, Category = "Hex|Grid")
    void BuildWorldNeighbors();

    // Récupère les voisins depuis le cache
    UFUNCTION(BlueprintPure, Category = "Hex|Grid")
    void GetNeighborsByWorld(const FHexAxialCoordinates &From, TArray<FHexAxialCoordinates> &Out) const;

    /** Si le premier hit est un acteur ‘Floor’, on ne crée PAS la tuile */
    UPROPERTY(EditAnywhere, Category = "Hex|Trace")
    bool bSkipTilesOverFloor = true;

    /** Nom/Tag de l’océan */
    UPROPERTY(EditAnywhere, Category = "Hex|Trace")
    FName FloorTag = "Floor";

    // Layout défauts (tes “bonnes valeurs”) — inchangé (ne modifie que le placement monde)
    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "1.0"))
    float TileSize = 250.f;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float XSpacingFactor = 0.3f;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float YSpacingFactor = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float RowOffsetFactor = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout")
    bool bOffsetOnQ = true;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout")
    FVector2D GlobalXYNudge = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout")
    FVector GridOrigin = FVector(750.f, 700.f, 0.f);

    /** Petit +Z global pour éviter le z-fighting (acteur relevé) */
    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float TileZOffset = 1.0f;

    // --- public --- (dans ta classe UHexGridManager)
    UPROPERTY(EditAnywhere, Category = "Hex|Generation")
    int32 GridRadius = 10;

    UPROPERTY(EditAnywhere, Category = "Hex|Generation")
    TSubclassOf<AHexTile> HexTileClass;

    // Bouton cliquable dans les détails (éditeur & en PIE) pour regénérer
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Hex|Generation")
    void RebuildGrid();

    /** Hauteur au-dessus d’où commence le trace */
    UPROPERTY(EditAnywhere, Category = "Hex|Trace", meta = (ClampMin = "0.0"))
    float TraceHeight = 1000.f;

    /** Profondeur en dessous jusqu’où descend le trace */
    UPROPERTY(EditAnywhere, Category = "Hex|Trace", meta = (ClampMin = "0.0"))
    float TraceDepth = 1000.f;

    /** Trace complexe (comme dans ton ancienne version) */
    UPROPERTY(EditAnywhere, Category = "Hex|Trace")
    bool bTraceComplex = true;

    /** Debug du trace (ligne + point d’impact) */
    UPROPERTY(EditAnywhere, Category = "Hex|Trace")
    bool bDebugTrace = true;

    /** Règle d'adjacence: si false, on interdit les voisins axiaux où Q et R changent simultanément (seulement 4 directions) */
    UPROPERTY(EditAnywhere, Category = "Hex|Rules")
    bool bAllowDiagonalAxialNeighbors = true;

private:
    /** Calcule la position finale (X,Y,Z) d’une tuile (Q,R) :
     *  - XY selon le layout (XSpacingFactor/YSpacingFactor + offset demi-ligne configurable)
     *  - Z par line trace (ECC_Visibility) + fallback Static/Dynamic + TileZOffset
     */
    FVector ComputeTileSpawnPosition(int32 Q, int32 R) const;

    bool TryComputeTileSpawnPosition(int32 Q, int32 R, FVector &OutLocation) const;

    /** Remap des indices génération -> coordonnées axiales attribuées (n'affecte pas la position monde) */
    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bInvertRAxisForLabels = false;

    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bInvertQAxisForLabels = false;

    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bSwapQRForLabels = false;

    /** Utiliser un repère "doubled-q" pour les coordonnées attribuées (voisins: (±2,0), (±1,±1)) */
    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bUseDoubledQForLabels = false;

    UPROPERTY(EditAnywhere, Category = "Hex|Data")
    TArray<FHexAxialCoordinates> ShopTiles; // coords en doubled-Q

    UFUNCTION(BlueprintCallable, Category = "Hex")
    void ApplySpecialTiles();

    FHexAxialCoordinates MapSpawnIndexToAxial(int32 Q, int32 R) const;

public:
    /** Distance entre A et B selon la convention courante (doubled-q ou non) */
    UFUNCTION(BlueprintPure, Category = "Hex|Query")
    int32 AxialDistance(const FHexAxialCoordinates &A, const FHexAxialCoordinates &B) const;

    /** Map interne Q,R → Actor de tuile */
    UPROPERTY()
    TMap<FHexAxialCoordinates, TWeakObjectPtr<AHexTile>> TilesMap;
    
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void BeginDestroy() override;
};
