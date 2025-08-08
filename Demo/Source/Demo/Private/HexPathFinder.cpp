#include "HexPathFinder.h"
#include "HexGridManager.h"
#include "Containers/Set.h"
#include "Containers/Map.h"
#include "Engine/World.h"

UHexPathFinder::UHexPathFinder()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHexPathFinder::BeginPlay()
{
    Super::BeginPlay();

    // Trouve automatiquement le composant UHexGridManager sur le même acteur
    GridManager = GetOwner()->FindComponentByClass<UHexGridManager>();
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("HexPathFinder : pas de UHexGridManager trouvé sur %s"), *GetOwner()->GetName());
    }
}

int32 UHexPathFinder::Heuristic(const FHexAxialCoordinates& A, const FHexAxialCoordinates& B) const
{
    // On utilise la distance « hexagonale » fournie par votre struct
    return A.DistanceTo(B);
}

TArray<FHexAxialCoordinates> UHexPathFinder::FindPath(const FHexAxialCoordinates& Start, const FHexAxialCoordinates& Goal) const
{
    if (!GridManager)
    {
        return {};
    }

    // A* : ensembles ouverts/fermés
    TSet<FHexAxialCoordinates> ClosedSet;
    TSet<FHexAxialCoordinates> OpenSet;
    OpenSet.Add(Start);

    // Scores g (coût depuis Start) et f = g + heuristique
    TMap<FHexAxialCoordinates, int32> GScore;
    GScore.Add(Start, 0);

    TMap<FHexAxialCoordinates, int32> FScore;
    FScore.Add(Start, Heuristic(Start, Goal));

    // Pour reconstruire le chemin
    TMap<FHexAxialCoordinates, FHexAxialCoordinates> CameFrom;

    while (OpenSet.Num() > 0)
    {
        // 1) Prendre node de OpenSet avec f le plus petit
        FHexAxialCoordinates Current;
        int32 BestF = TNumericLimits<int32>::Max();
        for (const auto& Coord : OpenSet)
        {
            int32 Score = FScore.Contains(Coord) ? FScore[Coord] : TNumericLimits<int32>::Max();
            if (Score < BestF)
            {
                BestF = Score;
                Current = Coord;
            }
        }

        // 2) Si c'est l'objectif, on reconstruit le chemin
        if (Current == Goal)
        {
            TArray<FHexAxialCoordinates> Path;
            FHexAxialCoordinates Node = Goal;
            while (CameFrom.Contains(Node))
            {
                Path.Insert(Node, 0);
                Node = CameFrom[Node];
            }
            Path.Insert(Start, 0);
            return Path;
        }

        // 3) Déplacer Current de OpenSet vers ClosedSet
        OpenSet.Remove(Current);
        ClosedSet.Add(Current);

        // 4) Explorer les voisins
        for (const auto& Neighbor : GridManager->GetNeighbors(Current))
        {
            if (ClosedSet.Contains(Neighbor))
            {
                continue;
            }

            const int32 TentativeG = GScore[Current] + Heuristic(Current, Neighbor);

            if (!OpenSet.Contains(Neighbor))
            {
                OpenSet.Add(Neighbor);
            }
            else if (TentativeG >= (GScore.Contains(Neighbor) ? GScore[Neighbor] : TNumericLimits<int32>::Max()))
            {
                continue;
            }

            // On a trouvé un meilleur chemin jusqu'à Neighbor
            CameFrom.Add(Neighbor, Current);
            GScore.Add(Neighbor, TentativeG);
            FScore.Add(Neighbor, TentativeG + Heuristic(Neighbor, Goal));
        }
    }

    // Aucune solution
    return {};
}
