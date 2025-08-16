// HexAnimationManager.cpp
#include "HexAnimationManager.h"
#include "HexPawn.h"  // Add this include

AHexAnimationManager::AHexAnimationManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AHexAnimationManager::RegisterPlayer(AHexPawn* Pawn)
{
    // Implementation
}

void AHexAnimationManager::UnregisterPlayer(AHexPawn* Pawn)
{
    // Implementation
}

void AHexAnimationManager::BatchUpdateAnimations()
{
    // Implementation
}