#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BattleActions.generated.h"

UENUM(BlueprintType)
enum class EBattleAction : uint8
{
    None   UMETA(DisplayName="None"),
    Attack UMETA(DisplayName="Attack"),
    Heal   UMETA(DisplayName="Heal"),
    Fireball UMETA(DisplayName="Fireball"),
    Defend UMETA(DisplayName="Defend"),
    LightningBolt UMETA(DisplayName="Lightning Bolt"),
    FullHeal UMETA(DisplayName="Full Heal"),

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
            case EBattleAction::Fireball:   return FText::FromString(TEXT("Fireball"));
            case EBattleAction::Defend: return FText::FromString(TEXT("Defend"));
            case EBattleAction::LightningBolt: return FText::FromString(TEXT("Lightning Bolt"));
            case EBattleAction::FullHeal: return FText::FromString(TEXT("Full Heal"));
            default:                    return FText::FromString(TEXT("-"));
        }
    }
};
