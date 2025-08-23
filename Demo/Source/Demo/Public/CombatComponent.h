#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleActions.h"

#include "CombatComponent.generated.h"

USTRUCT(BlueprintType)
struct FCombatStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    int32 HP = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    int32 MaxHP = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    int32 Attack = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    int32 Defense = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    int32 XP = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    int32 XPToNext = 100;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEMO_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    UFUNCTION(BlueprintCallable, Category="Combat")
    const FCombatStats& GetStats() const { return Stats; }

    UFUNCTION(BlueprintCallable, Category="Combat") void AddXP(int32 Amount);
    UFUNCTION(BlueprintCallable, Category="Combat") void ApplyDamage(int32 RawDamage);
    UFUNCTION(BlueprintCallable, Category="Combat") void Heal(int32 Amount);

	UFUNCTION(BlueprintPure, Category="Battle") int32 GetMaxSlots() const { return MaxSlots; }
    UFUNCTION(BlueprintPure, Category="Battle") const TArray<FBattleActionSlot>& GetLoadout() const { return Loadout; }
    UFUNCTION(BlueprintCallable, Category="Battle") void SetSlotAction(int32 Index, EBattleAction Action, int32 Cost = 1);
	UFUNCTION(BlueprintCallable, Category="Battle") void SetLoadout(const TArray<FBattleActionSlot>& In);

protected:
    virtual void BeginPlay() override;  // <-- add this

private:
    void TryLevelUp();

    UPROPERTY(EditDefaultsOnly, Category="Combat")
    FCombatStats Stats;

	UPROPERTY(EditAnywhere, Category="Battle")
    int32 MaxSlots = 5;

    UPROPERTY(EditAnywhere, Category="Battle")
    TArray<FBattleActionSlot> Loadout;
};
