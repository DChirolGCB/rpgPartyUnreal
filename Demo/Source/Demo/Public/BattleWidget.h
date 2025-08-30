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
class UWidgetAnimation;

UCLASS()
class DEMO_API UBattleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UBattleWidget(const FObjectInitializer&);

    UFUNCTION(BlueprintCallable, Category="Battle")
    void SetSides(UCombatComponent* InPlayer, UCombatComponent* InEnemy);

    UFUNCTION(BlueprintCallable, Category="Battle")
    void SetEnemyPortrait(UTexture2D* Tex);

    UFUNCTION(BlueprintCallable, Category="Battle")
    void SetPlayerPortrait(UTexture2D* Tex);

    UFUNCTION(BlueprintCallable, Category="Battle")
    void StartAutoBattle();

    UFUNCTION(BlueprintCallable, Category="Battle")
    void StopAutoBattle();

    UFUNCTION(BlueprintCallable, Category="Battle")
    void SetVictoryXP(int32 InXP) { VictoryXP = FMath::Max(0, InXP); }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void Refresh();
    void UpdateHighlights();
    void StepAction();
    void DoAction(UCombatComponent *Source, UCombatComponent *Target, const FBattleActionSlot &ActionSlot);
    
    void SpawnFloat(bool bOnEnemy, const FText &T, const FLinearColor &Color);
    void PlayHitWiggle(bool bOnEnemy);
    void UpdateDeathMasks();
    void GrantVictoryXP();

    UTextBlock *PlayerActTextAt(int32 Index) const;
    UTextBlock *EnemyActTextAt(int32 Index) const;

    UFUNCTION()
    void OnQuitClicked();

private:
    FTimerHandle RefreshTimer;
    FTimerHandle ActionTimer;

    UPROPERTY() UCombatComponent* PlayerCombat = nullptr;
    UPROPERTY() UCombatComponent* EnemyCombat  = nullptr;

    int32  CurrentIndex = 0;
    bool   bBattleRunning = false;
    bool   bPlayerTurn    = true;
    bool   bHighlightPlayerTurn = true;
    int32  HL_Player = INDEX_NONE;
    int32  HL_Enemy  = INDEX_NONE;

    int32  VictoryXP  = 0;
    bool   bXPGranted = false;

public: // Widgets
    // Player actions
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* PlayerAct0 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* PlayerAct1 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* PlayerAct2 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* PlayerAct3 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* PlayerAct4 = nullptr;

    // Enemy actions
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* EnemyAct0 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* EnemyAct1 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* EnemyAct2 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* EnemyAct3 = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* EnemyAct4 = nullptr;

    // Portraits
    UPROPERTY(meta=(BindWidgetOptional)) UImage* PlayerPortrait = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UImage* EnemyPortrait  = nullptr;

    // HP
    UPROPERTY(meta=(BindWidgetOptional)) UProgressBar* PlayerHPBar  = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock*   PlayerHPText = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UProgressBar* EnemyHPBar   = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock*   EnemyHPText  = nullptr;

    // Quit
    UPROPERTY(meta=(BindWidget)) UButton* BtnQuit = nullptr;

    // FX
    UPROPERTY(EditAnywhere, Category="Battle|FX") TSubclassOf<UFloatingTextWidget> FloatingTextClass;
    UPROPERTY(meta=(BindWidget)) UCanvasPanel* PlayerFXLayer = nullptr;
    UPROPERTY(meta=(BindWidget)) UCanvasPanel* EnemyFXLayer  = nullptr;
    UPROPERTY(Transient, meta=(BindWidgetAnimOptional)) UWidgetAnimation* PlayerHit = nullptr;
    UPROPERTY(Transient, meta=(BindWidgetAnimOptional)) UWidgetAnimation* EnemyHit  = nullptr;
    UPROPERTY(meta=(BindWidget)) UImage* PlayerDeathMask = nullptr;
    UPROPERTY(meta=(BindWidget)) UImage* EnemyDeathMask  = nullptr;
};
