// DemoGameMode.cpp

#include "DemoGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "HexPawn.h"

ADemoGameMode::ADemoGameMode()
{
    // Crée les deux composants runtime
    GridManager = CreateDefaultSubobject<UHexGridManager>(TEXT("HexGridManager"));
    PathFinder  = CreateDefaultSubobject<UHexPathFinder>(TEXT("HexPathFinder"));

    AddOwnedComponent(GridManager);
    AddOwnedComponent(PathFinder);
    if (!ensure(GridManager))  // vérifier que le component a bien été créé
    {
        UE_LOG(LogTemp, Error, TEXT("DemoGameMode constructor: GridManager is NULL!"));
    }
    static ConstructorHelpers::FClassFinder<AHexTile> TileBP(TEXT("/Game/Blueprints/BP_HexTile"));
    if (TileBP.Succeeded())
    {
        HexTileClass = TileBP.Class;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DemoGameMode: impossible de trouver /Game/Blueprints/BP_HexTile"));
    }

    ensure(HexTileClass);
    // On ne change pas les paramètres BlueprintPawnClass, HUDClass, etc.
}

void ADemoGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!ensure(GridManager))
    {
        UE_LOG(LogTemp, Error, TEXT("DemoGameMode BeginPlay: GridManager is NULL, aborting grid gen"));
        return;
    }
    if (!ensure(HexTileClass))
    {
        UE_LOG(LogTemp, Error, TEXT("DemoGameMode BeginPlay: HexTileClass is NULL, aborting grid gen"));
        return;
    }

    if (GridManager && *HexTileClass)
    {
        GridManager->InitializeGrid(GridRadius, HexTileClass);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DemoGameMode: GridManager ou HexTileClass invalide"));
        return;
    }

    // Log de vérif
    UE_LOG(LogTemp, Warning, TEXT("DemoGameMode BeginPlay : grille générée"));

    // Coordonnées de départ
    FHexAxialCoordinates StartCoords(0, 6);
    InitializePawnStartTile(StartCoords);
}

void ADemoGameMode::HandleTileClicked(AHexTile* ClickedTile)
{
    if (!ClickedTile || !GridManager || !PathFinder)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleTileClicked : pointeur null"));
        return;
    }

    // Convertit la tuile cliquée en coords
    FHexAxialCoordinates Target = ClickedTile->GetAxialCoordinates();

    AHexPawn* Pawn = Cast<AHexPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Pawn)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleTileClicked : pawn invalide"));
        return;
    }

    const AHexTile* CurrTile = Pawn->GetCurrentTile();
    if (!CurrTile)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleTileClicked : pawn sans tuile actuelle"));
        return;
    }

    FHexAxialCoordinates Start = CurrTile->GetAxialCoordinates();
    TArray<FHexAxialCoordinates> Path = PathFinder->FindPath(Start, Target);
    if (Path.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleTileClicked : aucun chemin trouvé"));
        return;
    }

    Pawn->StartPathFollowing(Path);
}

void ADemoGameMode::InitializePawnStartTile(const FHexAxialCoordinates& StartCoords)
{
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : GridManager null"));
        return;
    }

    AHexTile* Tile = GridManager->GetHexTileAt(StartCoords);
    if (!Tile)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : pas de tuile à (%d,%d)"),
               StartCoords.Q, StartCoords.R);
        return;
    }

    APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AHexPawn* HexP = Cast<AHexPawn>(P))
    {
        HexP->SetCurrentTile(Tile);
        UE_LOG(LogTemp, Warning, TEXT("Pawn démarré sur (%d,%d)"),
               StartCoords.Q, StartCoords.R);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : pawn non HexPawn"));
    }
}
