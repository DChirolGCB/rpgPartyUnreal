#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexCoordinates.h"
#include "HexTile.generated.h"

// === ENUM GLOBAL (obligatoire pour UHT) ===
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

UCLASS()
class DEMO_API AHexTile : public AActor
{
    GENERATED_BODY()

public:
    AHexTile();

    UFUNCTION(BlueprintPure, Category="Hex")
    UStaticMeshComponent* GetVisualMesh();

    UFUNCTION(BlueprintCallable, Category="Hex")
    void SetAxialCoordinates(const FHexAxialCoordinates& In) { Axial = In; }

    UFUNCTION(BlueprintCallable, Category="Hex")
    const FHexAxialCoordinates& GetAxialCoordinates() const { return Axial; }

    // --- Highlight ---
    UFUNCTION(BlueprintCallable, Category="Hex|Highlight")
    void SetHighlighted(bool bHighlight);

    UFUNCTION(BlueprintPure, Category="Hex|Highlight")
    bool IsHighlighted() const { return bIsHighlighted; }

    UFUNCTION(BlueprintCallable, Category="Hex|Type")
    void SetTileType(EHexTileType NewType);

    UFUNCTION(BlueprintPure, Category="Hex|Type")
    EHexTileType GetTileType() const { return TileType; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex")
    bool bIsShop = false;

protected:
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

    UFUNCTION() void HandleOnClicked(AActor* TouchedActor, FKey ButtonPressed);
    UFUNCTION() void HandleOnBeginCursorOver(AActor* TouchedActor);
    UFUNCTION() void HandleOnEndCursorOver(AActor* TouchedActor);

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Comp", meta=(AllowPrivateAccess="true"))
    USceneComponent* SceneRoot = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hex|Comp", meta=(AllowPrivateAccess="true"))
    UStaticMeshComponent* TileMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="Hex|Comp")
    FName VisualMeshName = TEXT("StaticMesh");

    UPROPERTY(EditDefaultsOnly, Category="Hex|Comp")
    FName VisualMeshTag = TEXT("HexVisual");

    UPROPERTY(Transient)
    UStaticMeshComponent* CachedVisualMesh = nullptr;

    // --- Data ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Data", meta=(DisplayName="Axial Coordinates"))
    FHexAxialCoordinates Axial;

    // --- Tile Type ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hex|Type")
    EHexTileType TileType = EHexTileType::Normal;

    UPROPERTY(EditAnywhere, Category="Hex|Type")
    FLinearColor TypeTint_Shop = FLinearColor(0.1f, 1.f, 0.1f, 1.f);

    // --- Highlight Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight")
    FLinearColor HighlightColor = FLinearColor(1.f, 1.f, 0.f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight")
    FLinearColor NormalColor = FLinearColor(1.f, 1.f, 1.f, 0.5f);

    // --- Emissive / Glow pour le highlight ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight")
    FLinearColor GlowColor = FLinearColor(1.f, 0.2f, 0.f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight", meta=(ClampMin="0.0"))
    float GlowStrengthOn = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex|Highlight", meta=(ClampMin="0.0"))
    float GlowStrengthOff = 0.0f;
private:
    bool bIsHighlighted = false;

    UPROPERTY() UMaterialInstanceDynamic* DynamicMaterial = nullptr;

    void UpdateMaterialColor();
};
