#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleActions.h"
#include "LoadoutEditorWidget.generated.h"

class UButton;
class UTextBlock;
class UCombatComponent;

UCLASS()
class DEMO_API ULoadoutEditorWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    ULoadoutEditorWidget(const FObjectInitializer&);
    UFUNCTION(BlueprintCallable, Category="Battle") void SetCombat(UCombatComponent* InCombat);

protected:
    virtual void NativeConstruct() override;
    virtual bool NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& DragDropEvent, UDragDropOperation* Op) override;

private:
    void RefreshSlots();
    int32 HitTestSlotIndex(const FVector2D& ScreenPos) const; // which slot rect under cursor

    // slot clicks still supported
    UFUNCTION() void OnSlot0(); UFUNCTION() void OnSlot1(); UFUNCTION() void OnSlot2(); UFUNCTION() void OnSlot3(); UFUNCTION() void OnSlot4();
    UFUNCTION() void OnSave();  UFUNCTION() void OnCancel();

private:
    UPROPERTY() UCombatComponent* Combat = nullptr;
    int32 SelectedSlot = 0;

    // working copy so Cancel can revert
    TArray<FBattleActionSlot> OriginalLoadout;
    TArray<FBattleActionSlot> EditorLoadout;

public: // BindWidget — left column
    UPROPERTY(meta=(BindWidget)) UButton*    SlotBtn0 = nullptr; UPROPERTY(meta=(BindWidget)) UTextBlock* SlotText0 = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*    SlotBtn1 = nullptr; UPROPERTY(meta=(BindWidget)) UTextBlock* SlotText1 = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*    SlotBtn2 = nullptr; UPROPERTY(meta=(BindWidget)) UTextBlock* SlotText2 = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*    SlotBtn3 = nullptr; UPROPERTY(meta=(BindWidget)) UTextBlock* SlotText3 = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*    SlotBtn4 = nullptr; UPROPERTY(meta=(BindWidget)) UTextBlock* SlotText4 = nullptr;

public: // BindWidget — right panel actions (replace your old buttons with ActionEntryWidget instances)
    UPROPERTY(meta=(BindWidget)) class UActionEntryWidget* ActAttack = nullptr;
    UPROPERTY(meta=(BindWidget)) class UActionEntryWidget* ActHeal   = nullptr;

public: // BindWidget — footer
    UPROPERTY(meta=(BindWidget)) UButton* BtnSave = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton* BtnCancel = nullptr;
};
