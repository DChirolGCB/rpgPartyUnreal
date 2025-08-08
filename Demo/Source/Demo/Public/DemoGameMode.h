// DemoGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexTile.h"
#include "HexPawn.h"
#include "HexGridManager.h"
#include "HexPathFinder.h"    // <— corriger le nom du header

#include "DemoGameMode.generated.h"

UCLASS()
class DEMO_API ADemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADemoGameMode();

    UHexGridManager* GetHexGridManager() const { return GridManager; }

    void HandleTileClicked(AHexTile* ClickedTile);

protected:
    virtual void BeginPlay() override;

    void InitializePawnStartTile(const FHexAxialCoordinates& StartCoords);

    UPROPERTY(EditDefaultsOnly, Category="Hex")
    TSubclassOf<AHexTile> HexTileClass;

    UPROPERTY(EditDefaultsOnly, Category="Hex")
    int32 GridRadius = 10;

    UPROPERTY()
    UHexGridManager* GridManager;

    UPROPERTY()
    UHexPathFinder* PathFinder;

};
