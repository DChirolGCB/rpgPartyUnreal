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

    UFUNCTION(BlueprintCallable, Category="Battle")
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
    bool  bXPGranted = false;

public: // BindWidget
    // Player
    UPROPERTY(meta = (BindWidget))
    UImage *PlayerPortrait = nullptr;
    UPROPERTY(meta = (BindWidget))
    UProgressBar *PlayerHPBar = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *PlayerHPText = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *PlayerAct0 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *PlayerAct1 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *PlayerAct2 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *PlayerAct3 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *PlayerAct4 = nullptr;

    // Enemy
    UPROPERTY(meta = (BindWidget))
    UImage *EnemyPortrait = nullptr;
    UPROPERTY(meta = (BindWidget))
    UProgressBar *EnemyHPBar = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *EnemyHPText = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *EnemyAct0 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *EnemyAct1 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *EnemyAct2 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *EnemyAct3 = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock *EnemyAct4 = nullptr;

    // Quit button
    UPROPERTY(meta = (BindWidget))
    UButton *BtnQuit = nullptr; // add a Button named BtnQuit in the BP
};
