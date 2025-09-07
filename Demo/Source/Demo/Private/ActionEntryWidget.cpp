#include "ActionEntryWidget.h"
#include "Components/TextBlock.h"
#include "BattleActions.h"
#include "ActionDragOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"  // DetectDragIfPressed
#include "InputCoreTypes.h"                    // EKeys

UActionEntryWidget::UActionEntryWidget(const FObjectInitializer& O) : Super(O) {}

void UActionEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Action == EBattleAction::None && DefaultAction != EBattleAction::None)
        Action = DefaultAction;
    RefreshLabel();
}

void UActionEntryWidget::RefreshLabel()
{
    if (Label) Label->SetText(UBattleActionLibrary::ActionToText(Action));
}

FReply UActionEntryWidget::NativeOnPreviewMouseButtonDown(const FGeometry& Geo, const FPointerEvent& E)
{
    if (E.GetEffectingButton() == EKeys::LeftMouseButton)
        return UWidgetBlueprintLibrary::DetectDragIfPressed(E, this, EKeys::LeftMouseButton).NativeReply;
    return Super::NativeOnPreviewMouseButtonDown(Geo, E);
}

void UActionEntryWidget::NativeOnDragDetected(const FGeometry& Geo,
                                              const FPointerEvent& E,
                                              UDragDropOperation*& OutOp)
{
    Super::NativeOnDragDetected(Geo, E, OutOp);

    // Use our payload op, not the generic one
    UActionDragOperation* Op = NewObject<UActionDragOperation>(this);
    Op->Pivot  = EDragPivot::MouseDown;
    Op->Action = Action;                        // <-- carry the action

    // Hit-test-invisible visual so drops reach the parent
    UActionEntryWidget* Proxy = CreateWidget<UActionEntryWidget>(GetWorld(), GetClass());
    if (Proxy)
    {
        if (Proxy->Label && Label) Proxy->Label->SetText(Label->GetText());
        Proxy->SetIsEnabled(false);
        Proxy->SetVisibility(ESlateVisibility::HitTestInvisible);
        Op->DefaultDragVisual = Proxy;
    }
    else
    {
        SetVisibility(ESlateVisibility::HitTestInvisible);
        Op->DefaultDragVisual = this;
    }

    OutOp = Op;
}

void UActionEntryWidget::NativeOnDragCancelled(const FDragDropEvent& Ev, UDragDropOperation* Op)
{
    Super::NativeOnDragCancelled(Ev, Op);
    SetVisibility(ESlateVisibility::Visible);
}

bool UActionEntryWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
    SetVisibility(ESlateVisibility::Visible);
    return Super::NativeOnDrop(Geo, Ev, Op);
}
