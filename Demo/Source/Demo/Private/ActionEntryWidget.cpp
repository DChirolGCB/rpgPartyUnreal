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

void UActionEntryWidget::NativeOnDragDetected(const FGeometry& Geo, const FPointerEvent& E, UDragDropOperation*& OutOp)
{
    UActionDragOperation* Op = NewObject<UActionDragOperation>(this);
    Op->Payload = this;
    Op->DefaultDragVisual = this; // ghost preview
    Op->Action = Action;
    OutOp = Op;
}
