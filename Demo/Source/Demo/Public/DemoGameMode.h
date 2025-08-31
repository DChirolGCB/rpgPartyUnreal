// DemoGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexCoordinates.h"
#include "Blueprint/UserWidget.h" // add (or: forward declare class UUserWidget;)
#include "DemoGameMode.generated.h"

// Forward declarations
class UUserWidget;
class AHexTile;
class UHexGridManager;
class UHexPathFinder;
class APathView;
class AHexPawn;
class AHexAnimationManager;
class UPlayerStatsWidget;
class UBattleWidget;
class ULoadoutEditorWidget;
class UEnemyDefinition;
class AHexEnemyPawn;
/**
 * Central GameMode: owns GridManager and PathFinder, drives click-to-move and path preview.
 */
UCLASS()
class DEMO_API ADemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADemoGameMode();

    /** Click handler entry point from tiles */
    UFUNCTION(BlueprintCallable, Category = "Hex|Input")
    void HandleTileClicked(AHexTile *ClickedTile);

    /** Accessors for managers */
    UFUNCTION(BlueprintPure, Category = "Hex")
    UHexGridManager *GetHexGridManager() const { return GridManager; }

    UFUNCTION(BlueprintPure, Category = "Hex")
    UHexPathFinder *GetHexPathFinder() const { return PathFinder; }

    /** Planned-path rendering */
    UFUNCTION(BlueprintCallable, Category = "Hex|Path")
    void ShowPlannedPathTo(AHexTile *GoalTile);

    UFUNCTION(BlueprintCallable, Category = "Hex|Path")
    void ClearPlannedPath();

    /** Hover preview controls */
    void PreviewPathTo(AHexTile *GoalTile);
    void ClearPreview();

    UFUNCTION(BlueprintCallable, Category = "Hex|PathPreview")
    void SetPreviewEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Hex|PathPreview")
    void TogglePreview();

    UFUNCTION(BlueprintPure, Category = "Hex|PathPreview")
    bool IsPreviewEnabled() const { return bPreviewEnabled; }

    /** Open shop UI at a given tile */
    UFUNCTION(BlueprintCallable, Category = "Hex|Gameplay")
    void OpenShopAt(AHexTile *ShopTile);

    /** Initial axial coordinates for the player pawn */
    UPROPERTY(EditAnywhere, Category = "Hex|Start")
    FHexAxialCoordinates StartCoords = FHexAxialCoordinates(0, 6);

    UFUNCTION(BlueprintCallable, Category = "Hex|Visibility")
    void UpdateReachableVisibility(int32 MaxSteps);

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> PlayerStatsWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void StartTestBattle();

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> BattleWidgetClass;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> LoadoutWidgetClass;

    UPROPERTY(EditAnywhere, Category="Battle")
    TMap<FName, TSoftObjectPtr<UEnemyDefinition>> EnemyCatalog;

    UPROPERTY(EditAnywhere, Category = "Battle")
    TSubclassOf<AHexEnemyPawn> EnemyPawnClass;

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void OpenLoadoutEditor();

    UFUNCTION() void OnPawnArrived(AHexPawn* Pawn);

        
protected:
    /** Engine lifecycle */
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Post-login hooks to finish initial snapping */
    virtual void PostLogin(APlayerController *NewPlayer) override;
    virtual void Logout(AController *Exiting) override;

    /** Position the pawn on the starting tile */
    void InitializePawnStartTile(const FHexAxialCoordinates &InStartCoords);

    /** Tile class used to spawn the grid */
    UPROPERTY(EditAnywhere, Category="Hex|Grid")
    TSubclassOf<class AHexTile> HexTileClass;

    /** Grid generation parameters */
    UPROPERTY(EditDefaultsOnly, Category = "Hex")
    int32 GridRadius = 10;

    /** Core managers (created/owned by GM) */
    UPROPERTY(VisibleAnywhere, Category = "Hex")
    UHexGridManager *GridManager = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Hex")
    UHexPathFinder *PathFinder = nullptr;

    /** Path debug actor */
    UPROPERTY()
    APathView *PathView = nullptr;

    /** Optional: UI class for shop */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> ShopWidgetClass;

    /** Optional PC override through BP */
    UPROPERTY(EditDefaultsOnly, Category = "Player")
    TSoftClassPtr<APlayerController> PCClassSoft;

    /** Optional animation manager */
    UPROPERTY()
    AHexAnimationManager *AnimationManager = nullptr;

private:
    /** Preview throttle */
    FTimerHandle PreviewThrottle;

    /** Hover target and caching to avoid recompute */
    TWeakObjectPtr<AHexTile> PendingGoal;
    FHexAxialCoordinates LastStart{INT32_MAX, INT32_MAX};
    FHexAxialCoordinates LastGoal{INT32_MAX, INT32_MAX};

    /** Keep a weak ref to shop widget to avoid double-destroy */
    UPROPERTY()
    TWeakObjectPtr<UUserWidget> ShopWidget;

    /** Periodic preview update */
    void DoPreviewTick();

    /** Typed pawn getter */
    AHexPawn *GetPlayerPawnTyped() const;

    /** Preview toggle */
    UPROPERTY(EditAnywhere, Category = "Hex|PathPreview")
    bool bPreviewEnabled = true;

    /** Post-login pawn snap retry */
    FTimerHandle SnapRetryHandle;

    /** Try to snap pawn and camera to the start tile */
    UFUNCTION()
    void TrySnapPawnOnce();

    TWeakObjectPtr<UUserWidget> PlayerStatsWidget;
    TWeakObjectPtr<UBattleWidget> BattleWidget;

    FRandomStream EnemyRNG;
    FName PickRandomEnemyIdFromCatalog() const;
};
