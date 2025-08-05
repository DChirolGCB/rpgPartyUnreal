#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexCoordinates.h"
#include "HexTile.generated.h"  // ← TOUJOURS DERNIER INCLUDE

UCLASS()
class DEMO_API AHexTile : public AActor
{
    GENERATED_BODY()

public:
    AHexTile();


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* TileMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex")
    FHexAxialCoordinates AxialCoords;

    UPROPERTY(BlueprintReadWrite, Category = "Hex")
    UHexGridManager* GridManager;

    UFUNCTION(BlueprintCallable, Category = "Hex")
    void SetGridManager(UHexGridManager* Manager);

    UFUNCTION(BlueprintCallable, Category = "Hex")
    FHexAxialCoordinates GetAxialCoordinates() const;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
    void SetCoordinates(FHexAxialCoordinates InCoords);
    const FHexAxialCoordinates& GetCoordinates() const { return AxialCoords; }
};
