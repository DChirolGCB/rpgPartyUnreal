#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleActions.h"
#include "BattleWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UButton;
class UCombatComponent;
class UTexture2D;
class UCanvasPanel;
class UFloatingTextWidget;
UCLASS()
class DEMO_API UBattleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UBattleWidget(const FObjectInitializer &);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SetSides(UCombatComponent *InPlayer, UCombatComponent *InEnemy);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SetEnemyPortrait(UTexture2D *Tex);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SetPlayerPortrait(UTexture2D *Tex); // optional symmetry

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void StartAutoBattle();

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void StopAutoBattle();

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SetVictoryXP(int32 InXP) { VictoryXP = FMath::Max(0, InXP); }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void Refresh();
    void UpdateHighlights();                        // <-- new
    UTextBlock *PlayerActTextAt(int32 Index) const; // <-- new
    UTextBlock *EnemyActTextAt(int32 Index) const;  // <-- new
    void StepAction();
    void DoAction(UCombatComponent *Source, UCombatComponent *Target, const FBattleActionSlot &ActionSlot);
    UFUNCTION()
    void OnQuitClicked(); // <-- new
    void SpawnFloat(bool bOnEnemy, const FText &T, const FLinearColor &Color);
    void PlayHitWiggle(bool bOnEnemy);
    bool bHighlightPlayerTurn = true;
    int32 HL_Player = INDEX_NONE;
    int32 HL_Enemy  = INDEX_NONE;

private:
    FTimerHandle RefreshTimer;
    FTimerHandle ActionTimer;

    UPROPERTY()
    UCombatComponent *PlayerCombat = nullptr;
    UPROPERTY()
    UCombatComponent *EnemyCombat = nullptr;

    int32 CurrentIndex = 0;
    bool bBattleRunning = false;
    bool bPlayerTurn = true; // one action per tick

    void GrantVictoryXP();
    int32 VictoryXP = 0;
    bool bXPGranted = false;

    void UpdateDeathMasks();

public: // BindWidget
    // Player
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *PlayerAct0 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *PlayerAct1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *PlayerAct2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *PlayerAct3 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *PlayerAct4 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *EnemyAct0 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *EnemyAct1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *EnemyAct2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *EnemyAct3 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock *EnemyAct4 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage *PlayerPortrait = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    UImage *EnemyPortrait = nullptr;


    // Player side
UPROPERTY(meta=(BindWidgetOptional)) UProgressBar* PlayerHPBar  = nullptr;
UPROPERTY(meta=(BindWidgetOptional)) UTextBlock*   PlayerHPText = nullptr;

// Enemy side
UPROPERTY(meta=(BindWidgetOptional)) UProgressBar* EnemyHPBar   = nullptr;
UPROPERTY(meta=(BindWidgetOptional)) UTextBlock*   EnemyHPText  = nullptr;
    // Quit button
    UPROPERTY(meta = (BindWidget))
    UButton *BtnQuit = nullptr; // add a Button named BtnQuit in the BP

    // FX layers
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    TSubclassOf<UFloatingTextWidget> FloatingTextClass;
    UPROPERTY(meta = (BindWidget))
    UCanvasPanel *PlayerFXLayer = nullptr;
    UPROPERTY(meta = (BindWidget))
    UCanvasPanel *EnemyFXLayer = nullptr;
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    class UWidgetAnimation *PlayerHit = nullptr;
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    class UWidgetAnimation *EnemyHit = nullptr;
    UPROPERTY(meta = (BindWidget))
    class UImage *PlayerDeathMask = nullptr;
    UPROPERTY(meta = (BindWidget))
    class UImage *EnemyDeathMask = nullptr;
};
