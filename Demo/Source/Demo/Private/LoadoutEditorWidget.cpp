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

    // Ensure the right-panel action entries reflect the available actions.
    auto TrySet = [&](UActionEntryWidget*& Ptr, const FName& Name, EBattleAction A)
    {
        if (!Ptr)
        {
            if (UWidget* Found = GetWidgetFromName(Name))
                Ptr = Cast<UActionEntryWidget>(Found);
        }
        if (Ptr) Ptr->SetAction(A);
    };

    TrySet(ActAttack, TEXT("ActAttack"), EBattleAction::Attack);
    TrySet(ActHeal, TEXT("ActHeal"), EBattleAction::Heal);
    TrySet(ActFireball, TEXT("ActFireball"), EBattleAction::Fireball);

    // optional new entries: find by name in UMG if present and set action
    if (UWidget* WDef = GetWidgetFromName(TEXT("ActDefend")))
        if (UActionEntryWidget* AW = Cast<UActionEntryWidget>(WDef)) AW->SetAction(EBattleAction::Defend);
    if (UWidget* WLin = GetWidgetFromName(TEXT("ActLightningBolt")))
        if (UActionEntryWidget* AW = Cast<UActionEntryWidget>(WLin)) AW->SetAction(EBattleAction::LightningBolt);
    if (UWidget* WFull = GetWidgetFromName(TEXT("ActFullHeal")))
        if (UActionEntryWidget* AW = Cast<UActionEntryWidget>(WFull)) AW->SetAction(EBattleAction::FullHeal);
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

void ULoadoutEditorWidget::SetSlotHighlight(int32 Index, bool /*bOn*/)
{
    auto Reset = [&](UButton* Btn, UTextBlock* Txt)
    {
        if (Btn) Btn->SetRenderOpacity(0.8f);
        if (Txt) Txt->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    };
    auto Hi = [&](UButton* Btn, UTextBlock* Txt)
    {
        if (Btn) Btn->SetRenderOpacity(1.0f);
        if (Txt) Txt->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
    };

    // reset all
    Reset(SlotBtn0, SlotText0);
    Reset(SlotBtn1, SlotText1);
    Reset(SlotBtn2, SlotText2);
    Reset(SlotBtn3, SlotText3);
    Reset(SlotBtn4, SlotText4);

    // highlight current
    switch (Index)
    {
        case 0: Hi(SlotBtn0, SlotText0); break;
        case 1: Hi(SlotBtn1, SlotText1); break;
        case 2: Hi(SlotBtn2, SlotText2); break;
        case 3: Hi(SlotBtn3, SlotText3); break;
        case 4: Hi(SlotBtn4, SlotText4); break;
        default: break;
    }
}

void ULoadoutEditorWidget::NativeOnDragEnter(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
    Super::NativeOnDragEnter(Geo, Ev, Op);
    HoveredSlot = HitTestSlotIndex(Ev.GetScreenSpacePosition());
    SetSlotHighlight(HoveredSlot, true);
    SetSlotsHitTest(false);               // <-- let root receive OnDrop
}

void ULoadoutEditorWidget::NativeOnDragLeave(const FDragDropEvent& Ev, UDragDropOperation* Op)
{
    Super::NativeOnDragLeave(Ev, Op);
    HoveredSlot = INDEX_NONE;
    SetSlotHighlight(INDEX_NONE, false);
    SetSlotsHitTest(true);                // <-- restore
}

bool ULoadoutEditorWidget::NativeOnDragOver(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
    const int32 NewIdx = HitTestSlotIndex(Ev.GetScreenSpacePosition());
    if (NewIdx != HoveredSlot)
    {
        HoveredSlot = NewIdx;
        SetSlotHighlight(HoveredSlot, true);
    }
    return true;
}

bool ULoadoutEditorWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
    SetSlotsHitTest(true);                // <-- restore
    SetSlotHighlight(INDEX_NONE, false);
    HoveredSlot = INDEX_NONE;

    const int32 Index = HitTestSlotIndex(Ev.GetScreenSpacePosition());
    if (!EditorLoadout.IsValidIndex(Index)) return false;

    if (UActionDragOperation* Drag = Cast<UActionDragOperation>(Op))
    {
        EditorLoadout[Index].Action   = Drag->Action;
        EditorLoadout[Index].SlotCost = 1;
        SelectedSlot = Index;
        RefreshSlots();

        if (UWidget* DV = Cast<UWidget>(Drag->DefaultDragVisual))
            DV->SetVisibility(ESlateVisibility::Visible);
        return true;
    }
    return false;
}


void ULoadoutEditorWidget::SetSlotsHitTest(bool bEnable)
{
    auto Set = [&](UButton* Btn)
    {
        if (!Btn) return;
        Btn->SetVisibility(bEnable ? ESlateVisibility::Visible
                                   : ESlateVisibility::SelfHitTestInvisible);
    };
    Set(SlotBtn0); Set(SlotBtn1); Set(SlotBtn2); Set(SlotBtn3); Set(SlotBtn4);
}