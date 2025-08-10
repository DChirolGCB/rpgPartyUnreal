// DemoGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexTile.h"
#include "HexPawn.h"
#include "HexGridManager.h"
#include "HexPathFinder.h"
#include "PathView.h"
#include "DemoGameMode.generated.h"

class UUserWidget;            // <-- AVANT la classe, au niveau global
class AHexTile;
class UHexGridManager;
class UHexPathFinder;
class APathView;
class AHexPawn;
/**
 * GameMode central : possède GridManager + PathFinder, gère le click-to-move + preview.
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

    // Getters
    UFUNCTION(BlueprintPure, Category="Hex")
    UHexGridManager* GetHexGridManager() const { return GridManager; }

    UFUNCTION(BlueprintPure, Category="Hex")
    UHexPathFinder* GetHexPathFinder() const { return PathFinder; }

    UFUNCTION(BlueprintCallable, Category="Hex|Path")
    void ShowPlannedPathTo(class AHexTile* GoalTile);

    UFUNCTION(BlueprintCallable, Category="Hex|Path")
    void ClearPlannedPath();

    // Preview
    void PreviewPathTo(class AHexTile* GoalTile);
    void ClearPreview();

    UFUNCTION(BlueprintCallable, Category="Hex|PathPreview")
    void SetPreviewEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Hex|PathPreview")
    void TogglePreview();

    UFUNCTION(BlueprintPure, Category="Hex|PathPreview")
    bool IsPreviewEnabled() const { return bPreviewEnabled; }

    // Gameplay
    UFUNCTION(BlueprintCallable, Category="UI|Shop")
void OpenShopAt(class AHexTile* ShopTile);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void InitializePawnStartTile(const FHexAxialCoordinates& StartCoords);

    // --- Params de génération ---
    UPROPERTY(EditDefaultsOnly, Category="Hex")
    TSubclassOf<AHexTile> HexTileClass;

    UPROPERTY(EditDefaultsOnly, Category="Hex")
    int32 GridRadius = 10;

    // --- Components possédés par le GameMode ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex", meta=(AllowPrivateAccess="true"))
    UHexGridManager* GridManager = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex", meta=(AllowPrivateAccess="true"))
    UHexPathFinder* PathFinder = nullptr;

    // Permet de forcer le PlayerController BP si nécessaire
    UPROPERTY(EditDefaultsOnly, Category="Classes")
    TSoftClassPtr<APlayerController> PCClassSoft;

private:
    UPROPERTY() APathView* PathView = nullptr;
    UPROPERTY(EditDefaultsOnly, Category="UI|Shop")
TSubclassOf<UUserWidget> ShopWidgetClass;
    UPROPERTY() TWeakObjectPtr<UUserWidget> ShopWidget;  // widget shop affiché
    

    FTimerHandle PreviewThrottle;
    TWeakObjectPtr<class AHexTile> PendingGoal;
    FHexAxialCoordinates LastStart{ INT32_MAX, INT32_MAX };
    FHexAxialCoordinates LastGoal { INT32_MAX, INT32_MAX };

    void DoPreviewTick();
    class AHexPawn* GetPlayerPawnTyped() const;

    UPROPERTY(EditAnywhere, Category="Hex|PathPreview")
    bool bPreviewEnabled = true;
};
