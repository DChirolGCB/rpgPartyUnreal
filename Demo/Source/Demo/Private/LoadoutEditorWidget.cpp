#include "LoadoutEditorWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "CombatComponent.h"
#include "BattleActions.h"
#include "ActionDragOperation.h"
#include "ActionEntryWidget.h"

ULoadoutEditorWidget::ULoadoutEditorWidget(const FObjectInitializer& O):Super(O){}

void ULoadoutEditorWidget::SetCombat(UCombatComponent* InCombat)
{
    Combat = InCombat;
    SelectedSlot = 0;

    OriginalLoadout = Combat ? Combat->GetLoadout() : TArray<FBattleActionSlot>{};
    EditorLoadout   = OriginalLoadout;
    if (EditorLoadout.Num() < 5) EditorLoadout.SetNum(5);
    RefreshSlots();
}

void ULoadoutEditorWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SlotBtn0) SlotBtn0->OnClicked.AddDynamic(this, &ULoadoutEditorWidget::OnSlot0);
    if (SlotBtn1) SlotBtn1->OnClicked.AddDynamic(this, &ULoadoutEditorWidget::OnSlot1);
    if (SlotBtn2) SlotBtn2->OnClicked.AddDynamic(this, &ULoadoutEditorWidget::OnSlot2);
    if (SlotBtn3) SlotBtn3->OnClicked.AddDynamic(this, &ULoadoutEditorWidget::OnSlot3);
    if (SlotBtn4) SlotBtn4->OnClicked.AddDynamic(this, &ULoadoutEditorWidget::OnSlot4);

    if (BtnSave)   BtnSave  ->OnClicked.AddDynamic(this, &ULoadoutEditorWidget::OnSave);
    if (BtnCancel) BtnCancel->OnClicked.AddDynamic(this, &ULoadoutEditorWidget::OnCancel);

    RefreshSlots();
}

void ULoadoutEditorWidget::RefreshSlots()
{
    auto SetOne = [](UTextBlock* T, const FBattleActionSlot& S)
    {
        if (T) T->SetText(UBattleActionLibrary::ActionToText(S.Action));
    };
    SetOne(SlotText0, EditorLoadout.IsValidIndex(0)?EditorLoadout[0]:FBattleActionSlot{});
    SetOne(SlotText1, EditorLoadout.IsValidIndex(1)?EditorLoadout[1]:FBattleActionSlot{});
    SetOne(SlotText2, EditorLoadout.IsValidIndex(2)?EditorLoadout[2]:FBattleActionSlot{});
    SetOne(SlotText3, EditorLoadout.IsValidIndex(3)?EditorLoadout[3]:FBattleActionSlot{});
    SetOne(SlotText4, EditorLoadout.IsValidIndex(4)?EditorLoadout[4]:FBattleActionSlot{});
}

int32 ULoadoutEditorWidget::HitTestSlotIndex(const FVector2D& ScreenPos) const
{
    const auto Under = [&](const UWidget* W)->bool{
        return W && W->GetCachedGeometry().IsUnderLocation(ScreenPos);
    };
    if (Under(SlotBtn0)) return 0;
    if (Under(SlotBtn1)) return 1;
    if (Under(SlotBtn2)) return 2;
    if (Under(SlotBtn3)) return 3;
    if (Under(SlotBtn4)) return 4;
    return INDEX_NONE;
}

bool ULoadoutEditorWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
    const FVector2D Screen = Ev.GetScreenSpacePosition();
    const int32 Index = HitTestSlotIndex(Screen);
    if (Index == INDEX_NONE) return false;

    if (UActionDragOperation* A = Cast<UActionDragOperation>(Op))
    {
        if (!EditorLoadout.IsValidIndex(Index)) return false;
        EditorLoadout[Index].Action = A->Action;
        EditorLoadout[Index].SlotCost = 1;
        SelectedSlot = Index;
        RefreshSlots();
        return true;
    }
    return false;
}

// slot selection
void ULoadoutEditorWidget::OnSlot0(){ SelectedSlot=0; }
void ULoadoutEditorWidget::OnSlot1(){ SelectedSlot=1; }
void ULoadoutEditorWidget::OnSlot2(){ SelectedSlot=2; }
void ULoadoutEditorWidget::OnSlot3(){ SelectedSlot=3; }
void ULoadoutEditorWidget::OnSlot4(){ SelectedSlot=4; }

void ULoadoutEditorWidget::OnSave()
{
    if (Combat) Combat->SetLoadout(EditorLoadout);
    RemoveFromParent();
}

void ULoadoutEditorWidget::OnCancel()
{
    // discard working copy; just close
    RemoveFromParent();
}
