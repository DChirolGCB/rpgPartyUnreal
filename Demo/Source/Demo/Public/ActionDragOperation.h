#pragma once
#include "Blueprint/DragDropOperation.h"
#include "BattleActions.h"
#include "ActionDragOperation.generated.h"

UCLASS()
class DEMO_API UActionDragOperation : public UDragDropOperation
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    EBattleAction Action = EBattleAction::None;
};
