// HexGridManager.h
#pragma once

#include "CoreMinimal.h"
#include "HexCoordinates.h"
#include "HexGridManager.generated.h"

class AHexTile;

/**
 * Builds and indexes a hex grid (axial Q,R; doubled-q labeling option).
 * - World XY placement follows your legacy layout (configurable offset-on-Q/R).
 * - World Z is sampled via a vertical line trace (Visibility with Static/Dynamic fallback).
 * - Provides queries: GetHexTileAt / GetNeighbors / AxialDistance.
 */
UCLASS(ClassGroup = (Hex), meta = (BlueprintSpawnableComponent))
class DEMO_API UHexGridManager : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Ctor */
    UHexGridManager();

    // ===================== Public API =====================

    /** Generate the grid (radius in tiles; tile actor class to spawn). */
    UFUNCTION(BlueprintCallable, Category = "Hex|Generation")
    void InitializeGrid(int32 Radius, TSubclassOf<AHexTile> TileClass);

    /** Rebuild the grid (callable from Details panel and at runtime). */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Hex|Generation")
    void RebuildGrid();

    /** Return tile actor at (Q,R) or nullptr if absent. */
    UFUNCTION(BlueprintCallable, Category = "Hex|Query")
    AHexTile *GetHexTileAt(const FHexAxialCoordinates &Coords) const;

    /** Return existing neighbors around (Q,R) (doubled-q axial deltas). */
    UFUNCTION(BlueprintCallable, Category = "Hex|Query")
    TArray<FHexAxialCoordinates> GetNeighbors(const FHexAxialCoordinates &Coords) const;

    /** Distance between A and B using the current convention (doubled-q or not). */
    UFUNCTION(BlueprintPure, Category = "Hex|Query")
    int32 AxialDistance(const FHexAxialCoordinates &A, const FHexAxialCoordinates &B) const;

    /** Rebuild the “by world position” neighbor cache (closest 6 in XY). */
    UFUNCTION(BlueprintCallable, Category = "Hex|Grid")
    void BuildWorldNeighbors();

    /** Copy neighbors from the world-position cache (empty if none). */
    UFUNCTION(BlueprintPure, Category = "Hex|Grid")
    void GetNeighborsByWorld(const FHexAxialCoordinates &From, TArray<FHexAxialCoordinates> &Out) const;

    /** Read-only access to internal tiles map (useful for tools). */
    const TMap<FHexAxialCoordinates, TWeakObjectPtr<AHexTile>> &GetHexTiles() const { return TilesMap; }

    // ===================== Editable Settings =====================

    // ---- Generation ----
    UPROPERTY(EditAnywhere, Category = "Hex|Generation")
    int32 GridRadius = 10;

    UPROPERTY(EditAnywhere, Category = "Hex|Generation")
    TSubclassOf<AHexTile> HexTileClass;

    // ---- Layout (legacy-friendly) ----
    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "1.0"))
    float TileSize = 250.f;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float XSpacingFactor = 0.3f;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float YSpacingFactor = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float RowOffsetFactor = 0.5f;

    /** true: offset on Q (odd-Q pointy-top); false: offset on R (odd-R flat-top). */
    UPROPERTY(EditAnywhere, Category = "Hex|Layout")
    bool bOffsetOnQ = true;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout")
    FVector2D GlobalXYNudge = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Hex|Layout")
    FVector GridOrigin = FVector(750.f, 700.f, 0.f);

    /** Small Z offset to avoid z-fighting for spawned tiles. */
    UPROPERTY(EditAnywhere, Category = "Hex|Layout", meta = (ClampMin = "0.0"))
    float TileZOffset = 1.0f;

    // ---- Trace (Z sampling) ----
    UPROPERTY(EditAnywhere, Category = "Hex|Trace", meta = (ClampMin = "0.0"))
    float TraceHeight = 1000.f;

    UPROPERTY(EditAnywhere, Category = "Hex|Trace", meta = (ClampMin = "0.0"))
    float TraceDepth = 1000.f;

    UPROPERTY(EditAnywhere, Category = "Hex|Trace")
    bool bTraceComplex = true;

    UPROPERTY(EditAnywhere, Category = "Hex|Trace")
    bool bSkipTilesOverFloor = true;

    UPROPERTY(EditAnywhere, Category = "Hex|Trace")
    FName FloorTag = "Floor";

    // ---- Coordinates / labeling (affects assigned axial coords, not world pos) ----
    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bInvertRAxisForLabels = false;

    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bInvertQAxisForLabels = false;

    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bSwapQRForLabels = false;

    /** Use doubled-q for labels (neighbors: (±2,0),(±1,±1)). */
    UPROPERTY(EditAnywhere, Category = "Hex|Coordinates")
    bool bUseDoubledQForLabels = false;

    // ---- Rules ----
    /** If false, restrict axial neighbors to 4 dirs (Q xor R changes); currently not used in GetNeighbors. */
    UPROPERTY(EditAnywhere, Category = "Hex|Rules")
    bool bAllowDiagonalAxialNeighbors = true;

    // ===================== Lifecycle =====================
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void BeginDestroy() override;

private:
    // ----- Internals -----
    /** World tile storage (Q,R → AHexTile). */
    UPROPERTY()
    TMap<FHexAxialCoordinates, TWeakObjectPtr<AHexTile>> TilesMap;

    /** Optional list of shop tiles (in doubled-q). */
    UPROPERTY(EditAnywhere, Category = "Hex|Data")
    TArray<FHexAxialCoordinates> ShopTiles;

    /** Neighbor cache by world proximity (keeps the 6 closest in XY). */
    TMap<FHexAxialCoordinates, TArray<FHexAxialCoordinates>> WorldNeighbors;

    // ----- Helpers -----
    /** Compute final world position for (Q,R) — XY by layout, Z by trace. */
    FVector ComputeTileSpawnPosition(int32 Q, int32 R) const;

    /** Compute world position; returns false if we decide to skip the tile. */
    bool TryComputeTileSpawnPosition(int32 Q, int32 R, FVector &OutLocation) const;

    /** Map generation indices -> assigned axial coordinates (labeling only). */
    FHexAxialCoordinates MapSpawnIndexToAxial(int32 Col, int32 Row) const;

    /** Apply tile-specific flags (e.g., mark shops). */
    UFUNCTION(BlueprintCallable, Category = "Hex")
    void ApplySpecialTiles();
};
