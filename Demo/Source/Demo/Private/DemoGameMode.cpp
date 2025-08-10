// DemoGameMode.cpp

#include "DemoGameMode.h"
#include "Algo/Reverse.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "HexPawn.h"

namespace HexBridge
{
    // BFS basé sur les voisins fournis par la grille (axial standard et tuiles existantes)
    static bool BuildBridge(UHexGridManager *GM,
                            const FHexAxialCoordinates &From,
                            const FHexAxialCoordinates &To,
                            TArray<FHexAxialCoordinates> &OutChain)
    {
        if (!GM)
            return false;
        if (From == To)
        {
            OutChain.Reset();
            return true;
        }

        TQueue<FHexAxialCoordinates> Open;
        TMap<FHexAxialCoordinates, FHexAxialCoordinates> Parent;

        Open.Enqueue(From);
        Parent.Add(From, From);

        bool bFound = false;
        FHexAxialCoordinates Cur;
        while (Open.Dequeue(Cur))
        {
            TArray<FHexAxialCoordinates> Neigh = GM->GetNeighbors(Cur);
            for (const FHexAxialCoordinates &N : Neigh)
            {
                if (Parent.Contains(N))
                    continue;
                // N existe garanti par GetNeighbors
                Parent.Add(N, Cur);
                if (N == To)
                {
                    bFound = true;
                    Cur = N;
                    break;
                }
                Open.Enqueue(N);
            }
            if (bFound)
                break;
        }

        if (!bFound)
            return false;

        // Reconstruit la chaîne (From exclus, To inclus)
        TArray<FHexAxialCoordinates> Chain;
        for (FHexAxialCoordinates C = Cur; !(C == From); C = Parent[C])
        {
            Chain.Add(C);
        }
        Algo::Reverse(Chain);
        OutChain = MoveTemp(Chain);
        return true;
    }

    // Force Path à n’avoir que des steps adjacents ; insère les intermédiaires manquants
    static void BridgePathUsingExistingTiles(UHexGridManager *GM, TArray<FHexAxialCoordinates> &Path)
    {
        if (!GM || Path.Num() < 2)
            return;

        TArray<FHexAxialCoordinates> Out;
        Out.Reserve(Path.Num() * 2);
        Out.Add(Path[0]);

        for (int32 i = 1; i < Path.Num(); ++i)
        {
            const FHexAxialCoordinates From = Out.Last();
            const FHexAxialCoordinates To = Path[i];

            // Déjà voisins selon la grille ?
            const bool bAdjacent = GM->GetNeighbors(From).Contains(To);
            if (bAdjacent)
            {
                Out.Add(To);
                continue;
            }

            // Sinon, on construit un petit pont via BFS (uniquement tuiles présentes)
            TArray<FHexAxialCoordinates> Bridge;
            if (BuildBridge(GM, From, To, Bridge) && Bridge.Num() > 0)
            {
                Out.Append(Bridge);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Bridge] Impossible de relier (%d,%d)->(%d,%d) par voisins existants — on ignore ce saut"),
                       From.Q, From.R, To.Q, To.R);
                // Ne pas ajouter To si non adjacent pour éviter des sauts impossibles
                // On garde Out comme tel et on continue (option: casser complètement)
            }
        }

        Path = MoveTemp(Out);
    }
}

namespace Hex
{
    // Doubled-Q neighbor set to match GridManager when bUseDoubledQForLabels = true
    static const FHexAxialCoordinates NeighborDirs[6] =
        {
            {+2, 0}, {+1, -1}, {-1, -1}, {-2, 0}, {-1, +1}, {+1, +1}};

    static int32 HexDistance(const FHexAxialCoordinates &A, const FHexAxialCoordinates &B)
    {
        // For doubled-q neighbors
        const int32 dq = FMath::Abs(A.Q - B.Q);
        const int32 dr = FMath::Abs(A.R - B.R);
        const int32 diag = FMath::Min(dq / 2, dr);
        const int32 remQ = dq - diag * 2;
        const int32 remR = dr - diag;
        return diag + remR + remQ / 2;
    }

    static bool AreNeighbors(const FHexAxialCoordinates &A, const FHexAxialCoordinates &B)
    {
        for (const auto &D : NeighborDirs)
        {
            if (A.Q + D.Q == B.Q && A.R + D.R == B.R)
                return true;
        }
        return false;
    }

    // Fait un pas dans la direction qui rapproche le plus de 'To'
    static FHexAxialCoordinates StepToward(const FHexAxialCoordinates &From, const FHexAxialCoordinates &To)
    {
        int32 BestIdx = 0, BestDist = TNumericLimits<int32>::Max();
        for (int32 i = 0; i < 6; ++i)
        {
            FHexAxialCoordinates C{From.Q + NeighborDirs[i].Q, From.R + NeighborDirs[i].R};
            const int32 Dist = HexDistance(C, To);
            if (Dist < BestDist)
            {
                BestDist = Dist;
                BestIdx = i;
            }
        }
        return {From.Q + NeighborDirs[BestIdx].Q, From.R + NeighborDirs[BestIdx].R};
    }

    // Garantit que chaque paire consécutive sont bien voisines (insère les intermédiaires si besoin)
    static void SanitizePath(TArray<FHexAxialCoordinates> &Path)
    {
        if (Path.Num() < 2)
            return;

        TArray<FHexAxialCoordinates> Out;
        Out.Reserve(Path.Num() * 2);
        Out.Add(Path[0]);

        for (int32 i = 1; i < Path.Num(); ++i)
        {
            FHexAxialCoordinates Cur = Out.Last();
            const FHexAxialCoordinates Target = Path[i];

            int32 Guard = 0;
            while (!AreNeighbors(Cur, Target) && Guard++ < 64)
            {
                const FHexAxialCoordinates Step = StepToward(Cur, Target);
                Out.Add(Step);
                Cur = Step;
            }

            Out.Add(Target);

#if !UE_BUILD_SHIPPING
            const int32 Jump = HexDistance(Out[Out.Num() - 2], Out.Last());
            if (Jump > 1)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SanitizePath] Jump resté à %d entre (%d,%d) -> (%d,%d)"),
                       Jump, Out[Out.Num() - 2].Q, Out[Out.Num() - 2].R, Out.Last().Q, Out.Last().R);
            }
#endif
        }

        Path = MoveTemp(Out);
    }
}

ADemoGameMode::ADemoGameMode()
{
    DefaultPawnClass = AHexPawn::StaticClass();
    // Crée les deux composants runtime
    GridManager = CreateDefaultSubobject<UHexGridManager>(TEXT("HexGridManager"));
    PathFinder = CreateDefaultSubobject<UHexPathFinder>(TEXT("HexPathFinder"));

    AddOwnedComponent(GridManager);
    AddOwnedComponent(PathFinder);
    if (!ensure(GridManager)) // vérifier que le component a bien été créé
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

    PathView = GetWorld()->SpawnActor<APathView>();
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
    APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }
}

void ADemoGameMode::HandleTileClicked(AHexTile *ClickedTile)
{
    if (!ClickedTile || !GridManager || !PathFinder)
        return;

    APawn *P = UGameplayStatics::GetPlayerPawn(this, 0);
    AHexPawn *HexP = Cast<AHexPawn>(P);
    if (!HexP)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleTileClicked: Pawn n'est pas un AHexPawn"));
        return;
    }

    AHexTile *FromTile = HexP->GetCurrentTile();
    if (!FromTile)
    {
        // Si pas encore de tuile de départ, on “spawn” l’état dessus
        HexP->SetCurrentTile(ClickedTile);
        HexP->SetActorLocation(ClickedTile->GetActorLocation());
        UE_LOG(LogTemp, Warning, TEXT("Affectation initiale de la tuile du Pawn -> (%d,%d)"),
               ClickedTile->GetAxialCoordinates().Q, ClickedTile->GetAxialCoordinates().R);
        return;
    }

    const FHexAxialCoordinates Start = FromTile->GetAxialCoordinates();
    const FHexAxialCoordinates Goal = ClickedTile->GetAxialCoordinates();

    if (Start == Goal)
        return;

    // A*
    TArray<FHexAxialCoordinates> Path = PathFinder->FindPath(Start, Goal);
    HexBridge::BridgePathUsingExistingTiles(GridManager, Path);

    if (Path.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("A*: aucun chemin"));
        return;
    }
    HexP->StartPathFollowing(Path, GridManager);
}

void ADemoGameMode::InitializePawnStartTile(const FHexAxialCoordinates &StartCoords)
{
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : GridManager null"));
        return;
    }

    AHexTile *Tile = GridManager->GetHexTileAt(StartCoords);
    if (!Tile)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : pas de tuile à (%d,%d)"),
               StartCoords.Q, StartCoords.R);
        return;
    }

    APawn *P = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AHexPawn *HexP = Cast<AHexPawn>(P))
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

void ADemoGameMode::ShowPlannedPathTo(AHexTile* GoalTile)
{
    if (!GridManager || !PathFinder || !GoalTile || !PathView) return;

    // Start = tuile actuelle du pawn
    AHexTile* StartTile = /* récupère ta tuile courante du pawn */ nullptr;
    if (!StartTile) return;

    const FHexAxialCoordinates Start = StartTile->GetAxialCoordinates();
    const FHexAxialCoordinates Goal  = GoalTile->GetAxialCoordinates();

    TArray<FHexAxialCoordinates> AxialPath = PathFinder->FindPath(Start, Goal);
    if (AxialPath.Num() < 2) { PathView->Clear(); return; }

    TArray<FVector> Points; Points.Reserve(AxialPath.Num());
    for (const auto& C : AxialPath)
        if (AHexTile* T = GridManager->GetHexTileAt(C))
            Points.Add(T->GetActorLocation());

    PathView->Show(Points);
}

void ADemoGameMode::ClearPlannedPath()
{
    if (PathView) PathView->Clear();
}