#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleActions.h"
#include "Blueprint/DragDropOperation.h"
#include "ActionEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class DEMO_API UActionEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UActionEntryWidget(const FObjectInitializer &);

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetAction(EBattleAction In)
	{
		Action = In;
		RefreshLabel();
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
	EBattleAction DefaultAction = EBattleAction::None; // set in BP if desired

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry &, const FPointerEvent &) override;
	virtual void NativeOnDragDetected(const FGeometry &Geo, const FPointerEvent &MouseEvent, UDragDropOperation *&OutOp) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent &Ev, UDragDropOperation *Op) override;
	virtual bool NativeOnDrop(const FGeometry &Geo, const FDragDropEvent &Ev, UDragDropOperation *Op) override;

private:
	void RefreshLabel();

private:
	UPROPERTY()
	EBattleAction Action = EBattleAction::None;

public: // BindWidget
	UPROPERTY(meta = (BindWidget))
	UTextBlock *Label = nullptr;
};
