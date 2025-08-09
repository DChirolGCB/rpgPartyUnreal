#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexCoordinates.h"        // FHexAxialCoordinates
#include "InputCoreTypes.h"        // FKey
#include "HexTile.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class DEMO_API AHexTile : public AActor
{
    GENERATED_BODY()

public:
    AHexTile();

    UFUNCTION(BlueprintCallable, Category="Hex")
    void SetAxialCoordinates(const FHexAxialCoordinates& In) { Axial = In; }

    UFUNCTION(BlueprintCallable, Category="Hex")
    const FHexAxialCoordinates& GetAxialCoordinates() const { return Axial; }

protected:
    virtual void PostInitializeComponents() override;

    UFUNCTION()
    void HandleOnClicked(AActor* TouchedActor, FKey ButtonPressed);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Comp")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Comp")
    TObjectPtr<UStaticMeshComponent> TileMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex|Data", meta=(DisplayName="Axial Coordinates"))
    FHexAxialCoordinates Axial;
};
