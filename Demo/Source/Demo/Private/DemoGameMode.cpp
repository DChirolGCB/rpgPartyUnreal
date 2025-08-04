#include "DemoGameMode.h"
#include "HexTile.h"

ADemoGameMode::ADemoGameMode()
{
}

void ADemoGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("DemoGameMode BeginPlay"));

    GridManager = NewObject<UHexGridManager>(this, UHexGridManager::StaticClass());
    if (GridManager)
    {
        GridManager->RegisterComponentWithWorld(GetWorld());
        GridManager->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        GridManager->InitializeGrid(GridRadius, HexTileClass);
        GridManager->GenerateGrid();

    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create GridManager!"));
    }
}
