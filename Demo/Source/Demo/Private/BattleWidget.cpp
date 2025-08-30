#include "BattleWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "CombatComponent.h"
#include "BattleActions.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "FloatingTextWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"


namespace
{
    inline void SetHP(UProgressBar *Bar, UTextBlock *Text, int32 HP, int32 MaxHP)
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

    inline void SetActs(const TArray<FBattleActionSlot> &L,
                        UTextBlock *T0, UTextBlock *T1, UTextBlock *T2, UTextBlock *T3, UTextBlock *T4)
    {
        auto SetOne = [](UTextBlock *T, const FBattleActionSlot &S)
        { if (T) T->SetText(UBattleActionLibrary::ActionToText(S.Action)); };

        SetOne(T0, L.IsValidIndex(0) ? L[0] : FBattleActionSlot{});
        SetOne(T1, L.IsValidIndex(1) ? L[1] : FBattleActionSlot{});
        SetOne(T2, L.IsValidIndex(2) ? L[2] : FBattleActionSlot{});
        SetOne(T3, L.IsValidIndex(3) ? L[3] : FBattleActionSlot{});
        SetOne(T4, L.IsValidIndex(4) ? L[4] : FBattleActionSlot{});
    }
}

UBattleWidget::UBattleWidget(const FObjectInitializer &O) : Super(O) {}

void UBattleWidget::SetSides(UCombatComponent *InPlayer, UCombatComponent *InEnemy)
{
    PlayerCombat = InPlayer;
    EnemyCombat = InEnemy;
    Refresh();
    StartAutoBattle();
}

void UBattleWidget::StartAutoBattle()
{
    if (!PlayerCombat || !EnemyCombat)
        return;
    CurrentIndex = 0;
    bPlayerTurn = true;
    bBattleRunning = true;
    bXPGranted = false; // <-- reset
    if (BtnQuit)
        BtnQuit->SetIsEnabled(false);
    UpdateHighlights();
    GetWorld()->GetTimerManager().SetTimer(ActionTimer, this, &UBattleWidget::StepAction, 0.8f, true, 0.0f);
}

void UBattleWidget::StopAutoBattle()
{
    bBattleRunning = false;
    if (UWorld *W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(ActionTimer);
    }
    if (BtnQuit)
        BtnQuit->SetIsEnabled(true);
}

void UBattleWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (BtnQuit)
        BtnQuit->OnClicked.AddDynamic(this, &UBattleWidget::OnQuitClicked);

    Refresh();
    UpdateHighlights();

    if (UWorld *W = GetWorld())
    {
        W->GetTimerManager().SetTimer(RefreshTimer, this, &UBattleWidget::Refresh, 0.10f, true);
    }
}

void UBattleWidget::NativeDestruct()
{
    if (UWorld *W = GetWorld())
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
    UpdateHighlights();
    UpdateDeathMasks();
}

static void ResetColor(UTextBlock *T)
{
    if (T)
        T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
}
static void Highlight(UTextBlock *T)
{
    if (T)
        T->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.2f, 1.f))); // soft yellow
}

UTextBlock *UBattleWidget::PlayerActTextAt(int32 Index) const
{
    switch (Index)
    {
    case 0:
        return PlayerAct0;
    case 1:
        return PlayerAct1;
    case 2:
        return PlayerAct2;
    case 3:
        return PlayerAct3;
    case 4:
        return PlayerAct4;
    default:
        return nullptr;
    }
}
UTextBlock *UBattleWidget::EnemyActTextAt(int32 Index) const
{
    switch (Index)
    {
    case 0:
        return EnemyAct0;
    case 1:
        return EnemyAct1;
    case 2:
        return EnemyAct2;
    case 3:
        return EnemyAct3;
    case 4:
        return EnemyAct4;
    default:
        return nullptr;
    }
}

void UBattleWidget::UpdateHighlights()
{
    // clear all
    ResetColor(PlayerAct0);
    ResetColor(PlayerAct1);
    ResetColor(PlayerAct2);
    ResetColor(PlayerAct3);
    ResetColor(PlayerAct4);
    ResetColor(EnemyAct0);
    ResetColor(EnemyAct1);
    ResetColor(EnemyAct2);
    ResetColor(EnemyAct3);
    ResetColor(EnemyAct4);

    if (!bBattleRunning)
        return;

    if (bPlayerTurn)
    {
        Highlight(PlayerActTextAt(CurrentIndex));
    }
    else
    {
        Highlight(EnemyActTextAt(CurrentIndex));
    }
}

void UBattleWidget::DoAction(UCombatComponent *Source, UCombatComponent *Target, const FBattleActionSlot &ActionSlot)
{
    if (!Source)
        return;

    switch (ActionSlot.Action)
    {
    case EBattleAction::Attack:
    {
        if (!Target)
            break;

        const bool bTargetIsEnemy = (Target == EnemyCombat); // <-- add this
        const int32 Dmg = Source->GetStats().Attack;

        Target->ApplyDamage(Dmg);
        PlayHitWiggle(bTargetIsEnemy); // now valid

        const FText Msg = FText::FromString(FString::Printf(TEXT("-%d"), Dmg));
        SpawnFloat(bTargetIsEnemy, Msg, FLinearColor(1.f, 0.25f, 0.25f));
    }
    break;

    case EBattleAction::Heal:
    {
        Source->Heal(3);

        const bool bSourceIsEnemy = (Source == EnemyCombat);
        const FText Msg = FText::FromString(TEXT("+3"));
        UE_LOG(LogTemp, Log, TEXT("[FX] %s: %s"),
               bSourceIsEnemy ? TEXT("enemy") : TEXT("player"),
               *Msg.ToString());
        SpawnFloat(bSourceIsEnemy, Msg, FLinearColor(0.25f, 1.f, 0.25f));
    }
    break;

    default:
        break;
    }
}

void UBattleWidget::StepAction()
{
    if (!bBattleRunning || !PlayerCombat || !EnemyCombat)
    {
        StopAutoBattle();
        return;
    }

    if (PlayerCombat->GetStats().HP <= 0 || EnemyCombat->GetStats().HP <= 0)
    {
        StopAutoBattle();
        return;
    }

    const TArray<FBattleActionSlot> &PL = PlayerCombat->GetLoadout();
    const TArray<FBattleActionSlot> &EL = EnemyCombat->GetLoadout();
    const int32 MaxTurns = FMath::Max(PL.Num(), EL.Num());
    if (CurrentIndex >= MaxTurns)
    {
        StopAutoBattle();
        return;
    }

    // highlight current
    UpdateHighlights();

    if (bPlayerTurn)
    {
        if (PL.IsValidIndex(CurrentIndex))
            DoAction(PlayerCombat, EnemyCombat, PL[CurrentIndex]);

        if (EnemyCombat->GetStats().HP <= 0)
        {
            GrantVictoryXP(); // <-- add here
            StopAutoBattle();
            return;
        }
        bPlayerTurn = false;
        return;
    }
    else
    {
        if (EL.IsValidIndex(CurrentIndex))
            DoAction(EnemyCombat, PlayerCombat, EL[CurrentIndex]);

        if (PlayerCombat->GetStats().HP <= 0)
        {
            StopAutoBattle(); // no XP on defeat
            return;
        }
        bPlayerTurn = true;
        ++CurrentIndex;
        if (CurrentIndex >= MaxTurns)
        {
            StopAutoBattle();
            return;
        }
    }
}

void UBattleWidget::OnQuitClicked()
{
    StopAutoBattle();

    if (APlayerController *PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        FInputModeGameAndUI Mode; // keep UI-style input
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true; // keep cursor visible
    }

    RemoveFromParent();
}

void UBattleWidget::SetEnemyPortrait(UTexture2D *Tex)
{
    if (EnemyPortrait && Tex)
    {
        EnemyPortrait->SetBrushFromTexture(Tex, /*bMatchSize*/ true);
        EnemyPortrait->SetColorAndOpacity(FLinearColor::White); // in case tint was non-white
    }
}

void UBattleWidget::SetPlayerPortrait(UTexture2D *Tex)
{
    if (PlayerPortrait && Tex)
    {
        PlayerPortrait->SetBrushFromTexture(Tex, true);
        PlayerPortrait->SetColorAndOpacity(FLinearColor::White);
    }
}

void UBattleWidget::GrantVictoryXP()
{
    if (!bXPGranted && PlayerCombat && VictoryXP > 0)
    {
        PlayerCombat->AddXP(VictoryXP);
        bXPGranted = true;
        UE_LOG(LogTemp, Log, TEXT("[Battle] Granted %d XP"), VictoryXP);
    }
}

void UBattleWidget::SpawnFloat(bool bOnEnemy, const FText &T, const FLinearColor &Color)
{
    if (!FloatingTextClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FX] FloatingTextClass null"));
        return;
    }

    UCanvasPanel *Layer = bOnEnemy ? EnemyFXLayer : PlayerFXLayer;

    UFloatingTextWidget *W = CreateWidget<UFloatingTextWidget>(GetWorld(), FloatingTextClass);
    if (!W)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FX] CreateWidget failed"));
        return;
    }

    W->SetTextAndColor(T, Color);

    if (Layer)
    {
        if (UCanvasPanelSlot *S = Layer->AddChildToCanvas(W))
        {
            S->SetAutoSize(true);
            S->SetZOrder(100); // au-dessus
            S->SetAnchors(FAnchors(0.5f, 0.5f));
            S->SetAlignment(FVector2D(0.5f, 0.5f));
            const float jx = FMath::FRandRange(-20.f, 20.f);
            const float jy = FMath::FRandRange(-6.f, 6.f);
            S->SetPosition(FVector2D(jx, -20.f + jy));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[FX] Layer null -> fallback viewport"));
        W->AddToViewport(9999);
    }

    W->PlayAndDie();
}

void UBattleWidget::PlayHitWiggle(bool bOnEnemy)
{
    if (UWidgetAnimation *A = bOnEnemy ? EnemyHit : PlayerHit)
        PlayAnimation(A, 0.f, 1);
}

void UBattleWidget::UpdateDeathMasks()
{
    const bool bPlayerDead = !PlayerCombat || PlayerCombat->GetStats().HP <= 0;
    const bool bEnemyDead  = !EnemyCombat  || EnemyCombat->GetStats().HP  <= 0;

    if (PlayerDeathMask)
        PlayerDeathMask->SetVisibility(bPlayerDead ? ESlateVisibility::HitTestInvisible
                                                    : ESlateVisibility::Collapsed);

    if (EnemyDeathMask)
        EnemyDeathMask->SetVisibility(bEnemyDead ? ESlateVisibility::HitTestInvisible
                                                 : ESlateVisibility::Collapsed);
}
