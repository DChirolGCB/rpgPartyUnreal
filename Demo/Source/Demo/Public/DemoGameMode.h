#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexGridManager.h"
#include "HexPathfinder.h"
#include "DemoGameMode.generated.h"

UCLASS()
class DEMO_API ADemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADemoGameMode();

    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Hex Grid")
    TSubclassOf<AHexTile> HexTileClass;

    UPROPERTY()
    UHexGridManager* GridManager;

    UPROPERTY()
    UHexPathfinder* Pathfinder;

    UPROPERTY(EditDefaultsOnly, Category = "Grid")
    int32 GridRadius = 5;
};
