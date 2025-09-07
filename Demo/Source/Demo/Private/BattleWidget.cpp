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
                        UTextBlock *T0, UTextBlock *T1, UTextBlock *T2, UTextBlock *T3, UTextBlock *T4,
                        int32 HighlightIdx)
    {
        auto SetOne = [&](UTextBlock *T, int32 Idx)
        {
            if (!IsValid(T))
                return; // safe contre nullptr / pending kill
            const FBattleActionSlot S = L.IsValidIndex(Idx) ? L[Idx] : FBattleActionSlot{};
            const FText Base = UBattleActionLibrary::ActionToText(S.Action);
            T->SetText(Idx == HighlightIdx
                           ? FText::FromString(FString::Printf(TEXT("▶ %s"), *Base.ToString()))
                           : Base);
        };
        SetOne(T0, 0);
        SetOne(T1, 1);
        SetOne(T2, 2);
        SetOne(T3, 3);
        SetOne(T4, 4);
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
    HL_Player = HL_Enemy = INDEX_NONE;
    if (!PlayerCombat || !EnemyCombat)
        return;
    CurrentIndex = 0;
    bPlayerTurn = true;
    bHighlightPlayerTurn = true; // highlight player first
    bBattleRunning = true;
    bXPGranted = false; // <-- reset
    if (BtnQuit)
        BtnQuit->SetIsEnabled(false);
    // UpdateHighlights();
    GetWorld()->GetTimerManager().SetTimer(ActionTimer, this, &UBattleWidget::StepAction, 0.5f, true, 0.0f);
}

void UBattleWidget::StopAutoBattle()
{
    HL_Player = HL_Enemy = INDEX_NONE;
    Refresh();
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

    // Remove the immediate Refresh() here. UI will refresh once SetSides() is called.
    // Refresh();

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

static void ClearActs(UTextBlock *T0, UTextBlock *T1, UTextBlock *T2, UTextBlock *T3, UTextBlock *T4)
{
    auto Clr = [&](UTextBlock *T)
    { if (IsValid(T)) T->SetText(FText::GetEmpty()); };
    Clr(T0);
    Clr(T1);
    Clr(T2);
    Clr(T3);
    Clr(T4);
}

void UBattleWidget::Refresh()
{
    // no computed highlight here; use HL_* only
    if (PlayerCombat)
    {
        const auto &S = PlayerCombat->GetStats();
        SetHP(PlayerHPBar, PlayerHPText, S.HP, S.MaxHP);
        SetActs(PlayerCombat->GetLoadout(),
                PlayerAct0, PlayerAct1, PlayerAct2, PlayerAct3, PlayerAct4,
                HL_Player);
    }
    else
    {
        ClearActs(PlayerAct0, PlayerAct1, PlayerAct2, PlayerAct3, PlayerAct4);
    }

    if (EnemyCombat)
    {
        const auto &S = EnemyCombat->GetStats();
        SetHP(EnemyHPBar, EnemyHPText, S.HP, S.MaxHP);
        SetActs(EnemyCombat->GetLoadout(),
                EnemyAct0, EnemyAct1, EnemyAct2, EnemyAct3, EnemyAct4,
                HL_Enemy);
    }
    else
    {
        ClearActs(EnemyAct0, EnemyAct1, EnemyAct2, EnemyAct3, EnemyAct4);
    }

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

    case EBattleAction::Fireball:
    {
        if (!Target)
            break;

        const bool bTargetIsEnemy = (Target == EnemyCombat);
        const int32 Dmg = Source->GetStats().Attack + 2; // fireball stronger

        Target->ApplyDamage(Dmg);
        PlayHitWiggle(bTargetIsEnemy);

        const FText Msg = FText::FromString(FString::Printf(TEXT("-%d"), Dmg));
        SpawnFloat(bTargetIsEnemy, Msg, FLinearColor(1.f, 0.5f, 0.0f)); // orange
    }

    default:
        break;
    }
}

void UBattleWidget::StepAction()
{
    HL_Player = INDEX_NONE;
    HL_Enemy  = INDEX_NONE;
    if (!bBattleRunning || !PlayerCombat || !EnemyCombat) { StopAutoBattle(); return; }
    if (PlayerCombat->GetStats().HP <= 0 || EnemyCombat->GetStats().HP <= 0) { StopAutoBattle(); return; }

    const TArray<FBattleActionSlot>& PL = PlayerCombat->GetLoadout();
    const TArray<FBattleActionSlot>& EL = EnemyCombat->GetLoadout();
    const int32 MaxTurns = FMath::Max(PL.Num(), EL.Num());
    if (CurrentIndex >= MaxTurns) { StopAutoBattle(); return; }

    if (bPlayerTurn)
    {
        HL_Player = CurrentIndex;   // ← add this
        Refresh();                  // show arrow on player BEFORE damage

        if (PL.IsValidIndex(CurrentIndex))
            DoAction(PlayerCombat, EnemyCombat, PL[CurrentIndex]);

        Refresh();                  // show damage with same highlight
        if (EnemyCombat->GetStats().HP <= 0)
        {
            GrantVictoryXP();       // ← add this
            StopAutoBattle();
            return;
        }

        bPlayerTurn = false;
        return;
    }
    else
    {
        HL_Enemy = CurrentIndex;    // already present
        Refresh();                  // show arrow on enemy BEFORE damage

        if (EL.IsValidIndex(CurrentIndex))
            DoAction(EnemyCombat, PlayerCombat, EL[CurrentIndex]);

        Refresh();
        if (PlayerCombat->GetStats().HP <= 0) { StopAutoBattle(); return; }

        bPlayerTurn = true;
        ++CurrentIndex;
        if (CurrentIndex >= MaxTurns) { StopAutoBattle(); return; }
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
    const bool bEnemyDead = !EnemyCombat || EnemyCombat->GetStats().HP <= 0;

    if (PlayerDeathMask)
        PlayerDeathMask->SetVisibility(bPlayerDead ? ESlateVisibility::HitTestInvisible
                                                   : ESlateVisibility::Collapsed);

    if (EnemyDeathMask)
        EnemyDeathMask->SetVisibility(bEnemyDead ? ESlateVisibility::HitTestInvisible
                                                 : ESlateVisibility::Collapsed);
}
