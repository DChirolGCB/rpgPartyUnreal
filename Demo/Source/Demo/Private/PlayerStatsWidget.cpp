#include "PlayerStatsWidget.h"
#include "Components/TextBlock.h"
#include "CombatComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UPlayerStatsWidget::UPlayerStatsWidget(const FObjectInitializer& Obj)
    : Super(Obj)
{
    // no ticking here
}

void UPlayerStatsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshTexts(); // initial fill

    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().SetTimer(
            RefreshTimer, this, &UPlayerStatsWidget::RefreshTexts,
            0.10f, /*bLoop=*/true
        );
    }
}

void UPlayerStatsWidget::NativeDestruct()
{
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(RefreshTimer);
    }
    Super::NativeDestruct();
}

void UPlayerStatsWidget::RefreshTexts()
{
    if (!Combat) return;
    const auto& S = Combat->GetStats();

    if (HPText)
        HPText->SetText(FText::FromString(
            FString::Printf(TEXT("HP: %d / %d"), S.HP, S.MaxHP)));

    if (AtkText)
        AtkText->SetText(FText::FromString(
            FString::Printf(TEXT("ATK: %d"), S.Attack)));

    if (DefText)
        DefText->SetText(FText::FromString(
            FString::Printf(TEXT("DEF: %d"), S.Defense)));

    if (XpText)
        XpText->SetText(FText::FromString(
            FString::Printf(TEXT("XP: %d / %d"), S.XP, S.XPToNext)));

    if (LvlText)
        LvlText->SetText(FText::FromString(
            FString::Printf(TEXT("LVL: %d"), S.Level)));
}
