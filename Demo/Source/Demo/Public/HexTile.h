#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexCoordinates.h"
#include "HexTile.generated.h"

/** Tile categories used for visuals and interactions */
UENUM(BlueprintType)
enum class EHexTileType : uint8
{
    Normal UMETA(DisplayName="Normal"),
    Shop   UMETA(DisplayName="Shop"),
    Spawn  UMETA(DisplayName="Spawn"),
    Goal   UMETA(DisplayName="Goal")
};

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class DEMO_API AHexTile : public AActor
{
    GENERATED_BODY()

public:
    /** Construct default components and set ticking policy */
    AHexTile();

    /** Returns the main visual mesh (cached) */
    UFUNCTION(BlueprintPure, Category="Hex")
    UStaticMeshComponent* GetVisualMesh();

    /** Sets axial grid coordinates */
    UFUNCTION(BlueprintCallable, Category="Hex")
    void SetAxialCoordinates(const FHexAxialCoordinates& In) { Axial = In; }

    /** Gets axial grid coordinates (const ref) */
    UFUNCTION(BlueprintPure, Category="Hex")
    const FHexAxialCoordinates& GetAxialCoordinates() const { return Axial; }

    /** Toggle highlight visuals (glow, outline, elevation) */
    UFUNCTION(BlueprintCallable, Category="Hex|Highlight")
    void SetHighlighted(bool bHighlight);

    /** Returns current highlight state */
    UFUNCTION(BlueprintPure, Category="Hex|Highlight")
    bool IsHighlighted() const { return bIsHighlighted; }

    /** Sets the tile type and updates visuals if needed */
    UFUNCTION(BlueprintCallable, Category="Hex|Type")
    void SetTileType(EHexTileType NewType);

    /** Returns the tile type */
    UFUNCTION(BlueprintPure, Category="Hex|Type")
    EHexTileType GetTileType() const { return TileType; }

    /** Optional flag to mark a tile as shop via editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex")
    bool bIsShop = false;

protected:
    /** Bind input-like events and initialize highlight hooks */
    virtual void PostInitializeComponents() override;

    /** Initialize materials, cache base Z, and prepare visuals */
    virtual void BeginPlay() override;

    /** Smooth elevation interpolation while highlighted */
    virtual void Tick(float DeltaSeconds) override;

    /** Click handler (opens shop or forwards to GameMode) */
    UFUNCTION() void HandleOnClicked(AActor* TouchedActor, FKey ButtonPressed);

    /** Hover begin handler (preview path + outline) */
    UFUNCTION() void HandleOnBeginCursorOver(AActor* TouchedActor);

    /** Hover end handler (clear preview + outline) */
    UFUNCTION() void HandleOnEndCursorOver(AActor* TouchedActor);

private:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Comp", meta=(AllowPrivateAccess="true"))
    USceneComponent* SceneRoot = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hex|Comp", meta=(AllowPrivateAccess="true"))
    UStaticMeshComponent* TileMesh = nullptr;

    UPROPERTY(Transient)
    UStaticMeshComponent* CachedVisualMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="Hex|Comp")
    FName VisualMeshName = TEXT("StaticMesh");

    UPROPERTY(EditDefaultsOnly, Category="Hex|Comp")
    FName VisualMeshTag = TEXT("HexVisual");

    // Data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Data", meta=(DisplayName="Axial Coordinates", AllowPrivateAccess="true"))
    FHexAxialCoordinates Axial;

    // Type & tint
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hex|Type", meta=(AllowPrivateAccess="true"))
    EHexTileType TileType = EHexTileType::Normal;

    UPROPERTY(EditAnywhere, Category="Hex|Type")
    FLinearColor TypeTint_Shop = FLinearColor(0.1f, 1.f, 0.1f, 1.f);

    // Highlight colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight", meta=(AllowPrivateAccess="true"))
    FLinearColor HighlightColor = FLinearColor(1.f, 1.f, 0.f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight", meta=(AllowPrivateAccess="true"))
    FLinearColor NormalColor = FLinearColor(1.f, 1.f, 1.f, 0.5f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight", meta=(AllowPrivateAccess="true"))
    FLinearColor GlowColor = FLinearColor(1.f, 0.2f, 0.f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
    float GlowStrengthOn = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
    float GlowStrengthOff = 0.0f;

    // Highlight elevation
    UPROPERTY(EditAnywhere, Category="Hex|Highlight")
    float HighlightLiftZ = 10.f;

    UPROPERTY(EditAnywhere, Category="Hex|Highlight", meta=(ClampMin="0.0"))
    float HighlightLerpSpeed = 12.f;

    // Runtime state
    UPROPERTY() float BaseZ = 0.f;
    UPROPERTY() float TargetZ = 0.f;
    UPROPERTY() bool  bElevInterpActive = false;
    UPROPERTY() bool  bIsHighlighted = false;
    UPROPERTY() UMaterialInstanceDynamic* DynamicMaterial = nullptr;

    /** Updates material parameters based on highlight state */
    void UpdateMaterialColor();
};
