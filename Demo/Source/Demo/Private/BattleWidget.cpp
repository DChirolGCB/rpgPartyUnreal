#include "BattleWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "CombatComponent.h"
#include "BattleActions.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
    inline void SetHP(UProgressBar* Bar, UTextBlock* Text, int32 HP, int32 MaxHP)
    {
        if (Bar)
        {
            const float Pct = MaxHP > 0 ? float(HP) / float(MaxHP) : 0.f;
            Bar->SetPercent(Pct);
        }
        if (Text)
        {
            Text->SetText(FText::FromString(FString::Printf(TEXT("HP: %d / %d"), HP, MaxHP)));
        }
    }

    inline void SetActs(const TArray<FBattleActionSlot>& L,
                        UTextBlock* T0, UTextBlock* T1, UTextBlock* T2, UTextBlock* T3, UTextBlock* T4)
    {
        auto SetOne = [](UTextBlock* T, const FBattleActionSlot& S)
        {
            if (T) T->SetText(UBattleActionLibrary::ActionToText(S.Action));
        };
        SetOne(T0, L.IsValidIndex(0) ? L[0] : FBattleActionSlot{});
        SetOne(T1, L.IsValidIndex(1) ? L[1] : FBattleActionSlot{});
        SetOne(T2, L.IsValidIndex(2) ? L[2] : FBattleActionSlot{});
        SetOne(T3, L.IsValidIndex(3) ? L[3] : FBattleActionSlot{});
        SetOne(T4, L.IsValidIndex(4) ? L[4] : FBattleActionSlot{});
    }
}

UBattleWidget::UBattleWidget(const FObjectInitializer& O) : Super(O) {}

void UBattleWidget::SetSides(UCombatComponent* InPlayer, UCombatComponent* InEnemy)
{
    PlayerCombat = InPlayer;
    EnemyCombat  = InEnemy;
    Refresh();
    // Auto-run for now
    StartAutoBattle();
}

void UBattleWidget::StartAutoBattle()
{
    if (!PlayerCombat || !EnemyCombat) return;
    CurrentIndex = 0;
    bPlayerTurn  = true;
    bBattleRunning = true;

    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().SetTimer(ActionTimer, this, &UBattleWidget::StepAction, 0.8f, true, 0.0f);
    }
}


void UBattleWidget::StopAutoBattle()
{
    bBattleRunning = false;
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(ActionTimer);
    }
}

void UBattleWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Refresh();
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().SetTimer(RefreshTimer, this, &UBattleWidget::Refresh, 0.10f, true);
    }
}

void UBattleWidget::NativeDestruct()
{
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(RefreshTimer);
        W->GetTimerManager().ClearTimer(ActionTimer);
    }
    Super::NativeDestruct();
}

void UBattleWidget::Refresh()
{
    if (PlayerCombat)
    {
        const FCombatStats& S = PlayerCombat->GetStats();
        SetHP(PlayerHPBar, PlayerHPText, S.HP, S.MaxHP);
        SetActs(PlayerCombat->GetLoadout(), PlayerAct0, PlayerAct1, PlayerAct2, PlayerAct3, PlayerAct4);
    }
    if (EnemyCombat)
    {
        const FCombatStats& S = EnemyCombat->GetStats();
        SetHP(EnemyHPBar, EnemyHPText, S.HP, S.MaxHP);
        SetActs(EnemyCombat->GetLoadout(), EnemyAct0, EnemyAct1, EnemyAct2, EnemyAct3, EnemyAct4);
    }
}

void UBattleWidget::DoAction(UCombatComponent* Source,
                             UCombatComponent* Target,
                             const FBattleActionSlot& ActionSlot) // renamed
{
    if (!Source) return;

    switch (ActionSlot.Action)
    {
        case EBattleAction::Attack:
            if (Target)
            {
                const int32 dmg = Source->GetStats().Attack;
                Target->ApplyDamage(dmg);
            }
            break;

        case EBattleAction::Heal:
            Source->Heal(3);
            break;

        default: break;
    }
}


void UBattleWidget::StepAction()
{
    if (!bBattleRunning || !PlayerCombat || !EnemyCombat) { StopAutoBattle(); return; }

    // stop if someone is dead
    if (PlayerCombat->GetStats().HP <= 0 || EnemyCombat->GetStats().HP <= 0)
    { StopAutoBattle(); return; }

    const TArray<FBattleActionSlot>& PL = PlayerCombat->GetLoadout();
    const TArray<FBattleActionSlot>& EL = EnemyCombat->GetLoadout();
    const int32 MaxTurns = FMath::Max(PL.Num(), EL.Num());
    if (CurrentIndex >= MaxTurns) { StopAutoBattle(); return; }

    if (bPlayerTurn)
    {
        if (PL.IsValidIndex(CurrentIndex))
            DoAction(PlayerCombat, EnemyCombat, PL[CurrentIndex]);

        // end if enemy died from player hit
        if (EnemyCombat->GetStats().HP <= 0) { StopAutoBattle(); return; }

        bPlayerTurn = false;   // next tick: enemy
        return;
    }
    else
    {
        if (EL.IsValidIndex(CurrentIndex))
            DoAction(EnemyCombat, PlayerCombat, EL[CurrentIndex]);

        // end if player died from enemy hit
        if (PlayerCombat->GetStats().HP <= 0) { StopAutoBattle(); return; }

        bPlayerTurn = true;    // next tick: player
        ++CurrentIndex;        // advance pair index
        if (CurrentIndex >= MaxTurns) { StopAutoBattle(); return; }
    }
}
