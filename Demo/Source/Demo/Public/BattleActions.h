#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BattleActions.generated.h"

UENUM(BlueprintType)
enum class EBattleAction : uint8
{
    None   UMETA(DisplayName="None"),
    Attack UMETA(DisplayName="Attack"),
    Heal   UMETA(DisplayName="Heal"),
    // add more later
};

USTRUCT(BlueprintType)
struct FBattleActionSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
    EBattleAction Action = EBattleAction::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
    int32 SlotCost = 1; // future-proof (multi-slot actions)
};

UCLASS()
class DEMO_API UBattleActionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Battle")
    static FText ActionToText(EBattleAction A)
    {
        switch (A)
        {
            case EBattleAction::Attack: return FText::FromString(TEXT("Attack"));
            case EBattleAction::Heal:   return FText::FromString(TEXT("Heal"));
            default:                    return FText::FromString(TEXT("-"));
        }
    }
};
