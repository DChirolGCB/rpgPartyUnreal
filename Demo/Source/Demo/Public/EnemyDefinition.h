#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatComponent.h"
#include "BattleActions.h"                 // <- for FBattleActionSlot
#include "EnemyDefinition.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class DEMO_API UEnemyDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy") FName EnemyId;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy") FText DisplayName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy") TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy") FCombatStats BaseStats;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy") int32 XPReward = 10;

    // NEW: action slots (size = 5 for now)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy")
    TArray<FBattleActionSlot> Loadout;
};
