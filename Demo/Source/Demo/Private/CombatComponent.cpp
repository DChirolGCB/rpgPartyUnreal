#include "CombatComponent.h"
#include "BattleActions.h"
#include "Math/UnrealMathUtility.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
	
	// defaults (can be overridden in BP defaults)
    Stats.Level = 1; Stats.MaxHP = 30; Stats.HP = 30;
    Stats.Attack = 7; Stats.Defense = 3; Stats.XP = 0; Stats.XPToNext = 50;

    MaxSlots = 5;
    Loadout.Init(FBattleActionSlot{}, MaxSlots);
    Loadout[0].Action = EBattleAction::Attack;
    Loadout[1].Action = EBattleAction::Attack;
    Loadout[2].Action = EBattleAction::Heal;
    Loadout[3].Action = EBattleAction::Attack;
    Loadout[4].Action = EBattleAction::Attack;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Loadout.Num() != MaxSlots) Loadout.SetNum(MaxSlots);
    for (auto& S : Loadout) if (S.SlotCost <= 0) S.SlotCost = 1;
}


void UCombatComponent::AddXP(int32 Amount)
{
    if (Amount <= 0) return;
    Stats.XP += Amount;
    TryLevelUp();
}

void UCombatComponent::SetLoadout(const TArray<FBattleActionSlot>& In)
{
    Loadout = In;
    if (Loadout.Num() != MaxSlots) Loadout.SetNum(MaxSlots);
}

void UCombatComponent::ApplyDamage(int32 RawDamage)
{
    const int32 dmg = FMath::Max(0, RawDamage - (Stats.Defense / 2));
    Stats.HP = FMath::Clamp(Stats.HP - dmg, 0, Stats.MaxHP);
}

void UCombatComponent::Heal(int32 Amount)
{
    if (Amount <= 0) return;
    Stats.HP = FMath::Clamp(Stats.HP + Amount, 0, Stats.MaxHP);
}

void UCombatComponent::TryLevelUp()
{
    while (Stats.XP >= Stats.XPToNext)
    {
        Stats.XP -= Stats.XPToNext;
        Stats.Level += 1;
        Stats.MaxHP += 5;
        Stats.Attack += 2;
        Stats.Defense += 1;
        Stats.HP = Stats.MaxHP;
        Stats.XPToNext = FMath::RoundToInt(Stats.XPToNext * 1.25f);
    }
}

void UCombatComponent::SetSlotAction(int32 Index, EBattleAction Action, int32 Cost)
{
    if (Cost <= 0) Cost = 1;
    if (Loadout.Num() != MaxSlots) Loadout.SetNum(MaxSlots);
    if (!Loadout.IsValidIndex(Index)) return;
    Loadout[Index].Action  = Action;
    Loadout[Index].SlotCost = Cost;
}
