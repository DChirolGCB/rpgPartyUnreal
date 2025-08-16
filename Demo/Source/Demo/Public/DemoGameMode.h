// DemoGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexTile.h"
#include "HexPawn.h"
#include "HexGridManager.h"
#include "HexPathFinder.h"
#include "PathView.h"

class UUserWidget; // <-- AVANT la classe, au niveau global
class AHexTile;
class UHexGridManager;
class UHexPathFinder;
class APathView;
class AHexPawn;
class AHexAnimationManager;

#include "DemoGameMode.generated.h"
/**
 * GameMode central : possède GridManager + PathFinder, gère le click-to-move + preview.
 */
UCLASS()
class DEMO_API ADemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADemoGameMode();

    UFUNCTION(BlueprintCallable, Category = "Hex|Input")
    void HandleTileClicked(class AHexTile *ClickedTile);

    UFUNCTION(BlueprintPure, Category = "Hex")
    UHexGridManager *GetHexGridManager() const { return GridManager; }

    UFUNCTION(BlueprintPure, Category = "Hex")
    UHexPathFinder *GetHexPathFinder() const { return PathFinder; }

    UFUNCTION(BlueprintCallable, Category = "Hex|Path")
    void ShowPlannedPathTo(AHexTile *GoalTile);

    UFUNCTION(BlueprintCallable, Category = "Hex|Path")
    void ClearPlannedPath();
    void PreviewPathTo(class AHexTile *GoalTile);
    void ClearPreview();

    UFUNCTION(BlueprintCallable, Category = "Hex|PathPreview")
    void SetPreviewEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Hex|PathPreview")
    void TogglePreview();

    UFUNCTION(BlueprintPure, Category = "Hex|PathPreview")
    bool IsPreviewEnabled() const { return bPreviewEnabled; }

    UFUNCTION(BlueprintCallable, Category = "Hex|Gameplay")
    void OpenShopAt(class AHexTile *ShopTile);

    UPROPERTY(EditAnywhere, Category="Hex|Start")
    FHexAxialCoordinates StartCoords = FHexAxialCoordinates(0, 6);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; // ← AJOUT

    void InitializePawnStartTile(const FHexAxialCoordinates& InStartCoords);

    UPROPERTY(EditDefaultsOnly, Category = "Hex")
    TSubclassOf<AHexTile> HexTileClass;

    UPROPERTY(EditDefaultsOnly, Category = "Hex")
    int32 GridRadius = 10;

    UPROPERTY(VisibleAnywhere, Category = "Hex")
    UHexGridManager *GridManager = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Hex")
    UHexPathFinder *PathFinder = nullptr;

    UPROPERTY()
    APathView *PathView = nullptr;

    // --- SHOP UI ---
    UPROPERTY(EditDefaultsOnly, Category = "UI") // ← AJOUT
    TSubclassOf<UUserWidget> ShopWidgetClass;    // ← AJOUT

    UPROPERTY(EditDefaultsOnly, Category="Player")
    TSoftClassPtr<APlayerController> PCClassSoft;

    UPROPERTY()
    AHexAnimationManager* AnimationManager = nullptr;
    
    int32 NumConnectedPlayers = 0;
private:
    FTimerHandle PreviewThrottle;
    TWeakObjectPtr<class AHexTile> PendingGoal;
    FHexAxialCoordinates LastStart{INT32_MAX, INT32_MAX};
    FHexAxialCoordinates LastGoal{INT32_MAX, INT32_MAX};

    // garder un weak vers le widget pour éviter les double-destroy
    UPROPERTY()
    TWeakObjectPtr<UUserWidget> ShopWidget; // ← AJOUT

    void DoPreviewTick();
    class AHexPawn *GetPlayerPawnTyped() const;

    UPROPERTY(EditAnywhere, Category = "Hex|PathPreview")
    bool bPreviewEnabled = true;

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    AHexPawn* SpawnPlayerPawn(APlayerController* ForPlayer) { return nullptr; } // Implement as needed
    void AssignPlayerVisuals(AHexPawn* Pawn, int32 PlayerIndex) {} // Implement as needed
    FTimerHandle SnapRetryHandle;

    UFUNCTION()
    void TrySnapPawnOnce();
    
};
