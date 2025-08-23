#include "HexEnemyPawn.h"
#include "EnemyDefinition.h"
#include "CombatComponent.h"
#include "Engine/Texture2D.h"

void AHexEnemyPawn::BeginPlay()
{
    Super::BeginPlay();
    if (EnemyData.IsNull() || !Combat) return;

    if (UEnemyDefinition* Def = EnemyData.LoadSynchronous())
    {
        Combat->SetStats(Def->BaseStats);

        // Apply loadout if provided, else fallback
        if (Def->Loadout.Num() > 0)
        {
            Combat->SetLoadout(Def->Loadout);
        }
        else
        {
            TArray<FBattleActionSlot> Fallback; Fallback.SetNum(5);
            for (auto& S : Fallback) { S.Action = EBattleAction::Attack; S.SlotCost = 1; }
            Combat->SetLoadout(Fallback);
        }

        // optional: if AHexPawn has a Portrait property, set it
        // Portrait = Def->Portrait;
    }
}

AHexEnemyPawn::AHexEnemyPawn()
{
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
    AutoPossessAI     = EAutoPossessAI::Disabled; // or AIController if you need AI later
    // no camera components here
}