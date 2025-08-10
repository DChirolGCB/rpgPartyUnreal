#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexCoordinates.h" // FHexAxialCoordinates
#include "InputCoreTypes.h" // FKey
#include "HexTile.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class DEMO_API AHexTile : public AActor
{
    GENERATED_BODY()

public:
    AHexTile();

    UFUNCTION(BlueprintPure, Category = "Hex")
    UStaticMeshComponent *GetVisualMesh();

    UFUNCTION(BlueprintCallable, Category = "Hex")
    void SetAxialCoordinates(const FHexAxialCoordinates &In) { Axial = In; }

    UFUNCTION(BlueprintCallable, Category = "Hex")
    const FHexAxialCoordinates &GetAxialCoordinates() const { return Axial; }

    // --- Fonctions de Highlight ---
    UFUNCTION(BlueprintCallable, Category = "Hex|Highlight")
    void SetHighlighted(bool bHighlight);

    UFUNCTION(BlueprintPure, Category = "Hex|Highlight")
    bool IsHighlighted() const { return bIsHighlighted; }

protected:
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

    // --- Événements de clic et hover ---
    UFUNCTION()
    void HandleOnClicked(AActor *TouchedActor, FKey ButtonPressed);

    UFUNCTION()
    void HandleOnBeginCursorOver(AActor *TouchedActor);

    UFUNCTION()
    void HandleOnEndCursorOver(AActor *TouchedActor);

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex|Comp", meta = (AllowPrivateAccess = "true"))
    USceneComponent *SceneRoot = nullptr;

    // Ne plus auto-créer. Laisser nul si tu utilises le mesh du BP.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex|Comp", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent *TileMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Hex|Comp")
    FName VisualMeshName = TEXT("StaticMesh");

    UPROPERTY(EditDefaultsOnly, Category = "Hex|Comp")
    FName VisualMeshTag = TEXT("HexVisual");

    UPROPERTY(Transient)
    UStaticMeshComponent *CachedVisualMesh = nullptr;
    // --- Data ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex|Data", meta = (DisplayName = "Axial Coordinates"))
    FHexAxialCoordinates Axial;

    // --- Highlight Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex|Highlight")
    FLinearColor HighlightColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); // Jaune par défaut

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex|Highlight")
    FLinearColor NormalColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f); // semi-transparent

    UPROPERTY(EditAnywhere, Category = "Hex|Highlight")
    FLinearColor GlowColor = FLinearColor(1.f, 0.2f, 0.f, 1.f); // rouge doux

    UPROPERTY(EditAnywhere, Category = "Hex|Highlight", meta = (ClampMin = "0.0"))
    float GlowStrengthOn = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Hex|Highlight", meta = (ClampMin = "0.0"))
    float GlowStrengthOff = 0.0f;

private:
    bool bIsHighlighted = false;

    // Matériel dynamique pour le highlight
    UPROPERTY()
    UMaterialInstanceDynamic *DynamicMaterial = nullptr;

    void UpdateMaterialColor();
};