#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleActions.h"      // EBattleAction, FBattleActionSlot
#include "BattleWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UCombatComponent;

UCLASS()
class DEMO_API UBattleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UBattleWidget(const FObjectInitializer&);

    UFUNCTION(BlueprintCallable, Category="Battle")
    void SetSides(UCombatComponent* InPlayer, UCombatComponent* InEnemy);

    UFUNCTION(BlueprintCallable, Category="Battle")
    void StartAutoBattle();

    UFUNCTION(BlueprintCallable, Category="Battle")
    void StopAutoBattle();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void Refresh();

    // Auto-battle
    void StepAction();

    void DoAction(class UCombatComponent* Source,
                  class UCombatComponent* Target,
                  const FBattleActionSlot& ActionSlot); // renamed

private: // timers
    FTimerHandle RefreshTimer;
    FTimerHandle ActionTimer;
	bool bPlayerTurn = true;  // true: player acts, false: enemy acts

private: // state
    UPROPERTY() UCombatComponent* PlayerCombat = nullptr;
    UPROPERTY() UCombatComponent* EnemyCombat  = nullptr;
    int32 CurrentIndex = 0;
    bool  bBattleRunning = false;

public: // BindWidget
    // Player
    UPROPERTY(meta=(BindWidget)) UImage*       PlayerPortrait = nullptr;
    UPROPERTY(meta=(BindWidget)) UProgressBar* PlayerHPBar    = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   PlayerHPText   = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   PlayerAct0 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   PlayerAct1 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   PlayerAct2 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   PlayerAct3 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   PlayerAct4 = nullptr;

    // Enemy
    UPROPERTY(meta=(BindWidget)) UImage*       EnemyPortrait = nullptr;
    UPROPERTY(meta=(BindWidget)) UProgressBar* EnemyHPBar    = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   EnemyHPText   = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   EnemyAct0 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   EnemyAct1 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   EnemyAct2 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   EnemyAct3 = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*   EnemyAct4 = nullptr;
};
