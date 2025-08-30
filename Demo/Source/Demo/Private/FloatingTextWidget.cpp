#include "FloatingTextWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimationEvents.h"  // add

void UFloatingTextWidget::SetTextAndColor(const FText& T, const FLinearColor& C)
{
    if (Label) { Label->SetText(T); Label->SetColorAndOpacity(C); }
}

void UFloatingTextWidget::PlayAndDie()
{
    if (Pop)
    {
        FWidgetAnimationDynamicEvent End;
        End.BindDynamic(this, &UFloatingTextWidget::OnAnimFinished); // fix
        BindToAnimationFinished(Pop, End);
        PlayAnimation(Pop, 0.f, 1);
    }
    else
    {
        FTimerHandle H;
        GetWorld()->GetTimerManager().SetTimer(H, [this]{ RemoveFromParent(); }, 0.8f, false);
    }
}

void UFloatingTextWidget::OnAnimFinished()
{
    RemoveFromParent();
}
