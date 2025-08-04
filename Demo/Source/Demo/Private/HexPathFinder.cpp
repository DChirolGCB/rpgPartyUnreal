#include "HexPathfinder.h"
#include "HexGridManager.h"

UHexPathfinder::UHexPathfinder()
{
}

void UHexPathfinder::Initialize(UHexGridManager* InGridManager)
{
    GridManager = InGridManager;
}

TArray<FHexAxialCoordinates> UHexPathfinder::FindPath(const FHexAxialCoordinates& Start, const FHexAxialCoordinates& Goal)
{
    UE_LOG(LogTemp, Warning, TEXT("FindPath - Start: (%d, %d), Goal: (%d, %d)"), Start.Q, Start.R, Goal.Q, Goal.R);

    if (!GridManager.IsValid())
    {
        return TArray<FHexAxialCoordinates>();
    }

    if (Start == Goal)
    {
        return { Start };
    }

    if (!GridManager->GetHexTiles().Contains(Start))
    {
        UE_LOG(LogTemp, Error, TEXT("Start (%d, %d) NOT in HexTiles!"), Start.Q, Start.R);
    }
    if (!GridManager->GetHexTiles().Contains(Goal))
    {
        UE_LOG(LogTemp, Error, TEXT("Goal (%d, %d) NOT in HexTiles!"), Goal.Q, Goal.R);
    }

    // A* Algorithm
    TArray<FHexAxialCoordinates> OpenSet;
    TSet<FHexAxialCoordinates> ClosedSet;
    TMap<FHexAxialCoordinates, float> GScore;
    TMap<FHexAxialCoordinates, float> FScore;
    TMap<FHexAxialCoordinates, FHexAxialCoordinates> CameFrom;

    OpenSet.Add(Start);
    GScore.Add(Start, 0.0f);
    FScore.Add(Start, CalculateHeuristic(Start, Goal));

    while (OpenSet.Num() > 0)
    {
        // Trouve le nœud avec le plus petit FScore
        int32 CurrentIndex = 0;
        for (int32 i = 1; i < OpenSet.Num(); i++)
        {
            if (FScore[OpenSet[i]] < FScore[OpenSet[CurrentIndex]])
            {
                CurrentIndex = i;
            }
        }

        FHexAxialCoordinates Current = OpenSet[CurrentIndex];

        if (Current == Goal)
        {
            return ReconstructPath(Goal, CameFrom);
        }

        OpenSet.RemoveAt(CurrentIndex);
        ClosedSet.Add(Current);

        TArray<FHexAxialCoordinates> Neighbors = GridManager->GetNeighbors(Current);
        for (const FHexAxialCoordinates& Neighbor : Neighbors)
        {
            if (ClosedSet.Contains(Neighbor))
            {
                continue;
            }

            float TentativeGScore = GScore[Current] + 1.0f; // Coût uniforme pour l'instant

            if (!OpenSet.Contains(Neighbor))
            {
                OpenSet.Add(Neighbor);
            }
            else if (TentativeGScore >= GScore.FindRef(Neighbor))
            {
                continue;
            }

            CameFrom.Add(Neighbor, Current);
            GScore.Add(Neighbor, TentativeGScore);
            FScore.Add(Neighbor, TentativeGScore + CalculateHeuristic(Neighbor, Goal));
        }
    }

    return TArray<FHexAxialCoordinates>(); // Aucun chemin trouvé
}

float UHexPathfinder::CalculateHeuristic(const FHexAxialCoordinates& Start, const FHexAxialCoordinates& Goal) const
{
    return Start.DistanceTo(Goal);
}

TArray<FHexAxialCoordinates> UHexPathfinder::ReconstructPath(const FHexAxialCoordinates& Goal, const TMap<FHexAxialCoordinates, FHexAxialCoordinates>& CameFrom)
{
    TArray<FHexAxialCoordinates> Path;
    FHexAxialCoordinates Current = Goal;

    Path.Add(Current);

    while (CameFrom.Contains(Current))
    {
        Current = CameFrom[Current];
        Path.Insert(Current, 0);
    }

    return Path;
}