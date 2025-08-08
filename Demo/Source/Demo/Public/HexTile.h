#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexCoordinates.h" // Votre struct FHexAxialCoordinates + GetTypeHash
#include "HexTile.generated.h"

UCLASS()
class DEMO_API AHexTile : public AActor
{
    GENERATED_BODY()

public:
    AHexTile();

    /** Définit les coordonnées axiales de cette tuile */
    UFUNCTION(BlueprintCallable, Category = "Hex")
    void SetAxialCoordinates(const FHexAxialCoordinates &Coordinates);

    /** Retourne les coordonnées axiales */
    UFUNCTION(BlueprintCallable, Category = "Hex")
    FHexAxialCoordinates GetAxialCoordinates() const { return AxialCoordinates; }

    UPROPERTY(EditAnywhere, Category = "Hex|Visual", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float VisualZOffset = 1.0f; // 0.5 à 2.0 marche bien selon tes meshes

protected:
    /** Coordonnées axiales stockées */
    UPROPERTY(VisibleAnywhere, Category = "Hex")
    FHexAxialCoordinates AxialCoordinates;

    /** Maillage de la tuile */
    UPROPERTY(VisibleAnywhere, Category = "Hex")
    UStaticMeshComponent *TileMesh;

    virtual void PostInitializeComponents() override;

    /** Handler C++ du clic, lié à OnClicked de l’acteur */
    UFUNCTION()
    void HandleOnClicked(AActor *TouchedActor, FKey ButtonPressed);
    virtual void OnConstruction(const FTransform &Transform) override;
};
