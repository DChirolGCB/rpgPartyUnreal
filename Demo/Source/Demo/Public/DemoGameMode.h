// DemoGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexTile.h"
#include "HexPawn.h"
#include "HexGridManager.h"
#include "HexPathFinder.h"
#include "DemoGameMode.generated.h"

/**
 * GameMode central : possède GridManager + PathFinder, gère le click-to-move.
 */
UCLASS()
class DEMO_API ADemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADemoGameMode();

    /** Appelé par AHexTile::HandleOnClicked */
    UFUNCTION(BlueprintCallable, Category="Hex|Input")
    void HandleTileClicked(class AHexTile* ClickedTile);

    // Getters propres (utilisés par du code existant)
    UFUNCTION(BlueprintPure, Category="Hex")
    UHexGridManager* GetHexGridManager() const { return GridManager; }

    UFUNCTION(BlueprintPure, Category="Hex")
    UHexPathFinder*  GetHexPathFinder() const { return PathFinder; }

protected:
    virtual void BeginPlay() override;

    void InitializePawnStartTile(const FHexAxialCoordinates& StartCoords);

    // --- Params de génération ---
    UPROPERTY(EditDefaultsOnly, Category="Hex")
    TSubclassOf<AHexTile> HexTileClass;

    UPROPERTY(EditDefaultsOnly, Category="Hex")
    int32 GridRadius = 10;

    // --- Components possédés par le GameMode ---
    UPROPERTY()
    UHexGridManager* GridManager;

    UPROPERTY()
    UHexPathFinder* PathFinder;
};
