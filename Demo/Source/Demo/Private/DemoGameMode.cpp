#include "DemoGameMode.h"
#include "HexTile.h"
#include "HexPawn.h"
#include "HexPathfinder.h"
#include "HexGridManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h" // Pour TActorIterator

ADemoGameMode::ADemoGameMode()
{
    GridManager = CreateDefaultSubobject<UHexGridManager>(TEXT("HexGridManager"));
    AddOwnedComponent(GridManager); // optionnel mais plus propre
}

void ADemoGameMode::BeginPlay()
{
     Super::BeginPlay();

    Pathfinder = NewObject<UHexPathfinder>(this);

    // 🔁 Mieux : recherche l’acteur avec le composant UHexGridManager
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (UHexGridManager* FoundGridManager = It->FindComponentByClass<UHexGridManager>())
        {
            GridManager = FoundGridManager;
            break;
        }
    }

    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find GridManager!"));
        return;
    }

    if (!HexTileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("HexTileClass is not set!"));
        return;
    }

    GridManager->InitializeGrid(GridRadius, HexTileClass);
    GridManager->GenerateGrid();
}

void ADemoGameMode::HandleTileClicked(AHexTile* ClickedTile)
{
    if (!ClickedTile || !GridManager || !Pathfinder)
    {
        UE_LOG(LogTemp, Warning, TEXT("Missing references in HandleTileClicked"));
        return;
    }

    // Récupérer le pawn du joueur
    AHexPawn* HexPawn = Cast<AHexPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!HexPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("No AHexPawn found!"));
        return;
    }

    // Coordonnées de départ
    AHexTile* StartTile = HexPawn->GetCurrentTile();
    if (!StartTile)
    {
        UE_LOG(LogTemp, Warning, TEXT("No start tile assigned to the pawn"));
        return;
    }

    FHexAxialCoordinates StartCoords = StartTile->GetCoordinates();
    FHexAxialCoordinates TargetCoords = ClickedTile->GetCoordinates();

    // Générer un chemin
    Pathfinder->SetGridManager(GridManager);
    TArray<FHexAxialCoordinates> Path = Pathfinder->FindPath(StartCoords, TargetCoords);

    if (Path.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid path found."));
        return;
    }

    // Démarrer le déplacement du pawn
    HexPawn->StartPathFollowing(Path);
}

UHexGridManager *ADemoGameMode::GetHexGridManager() const
{
    return GridManager;
}