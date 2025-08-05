// DemoGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DemoGameMode.generated.h"

// Forward declarations
class UHexGridManager;
class AHexTile;
class UHexPathfinder;

UCLASS()
class DEMO_API ADemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADemoGameMode();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void HandleTileClicked(AHexTile* ClickedTile);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    UHexGridManager* GetHexGridManager() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    TSubclassOf<AHexTile> HexTileClass;

    // ✅ Cette ligne est la clé :
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ExposeOnSpawn = true))
    UHexGridManager* GridManager;

    UPROPERTY()
    UHexPathfinder* Pathfinder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridRadius = 5;
};
