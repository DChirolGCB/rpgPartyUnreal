#include "HexPathFinder.h"
#include "HexGridManager.h"
#include "Algo/Reverse.h"

UHexPathFinder::UHexPathFinder()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHexPathFinder::GetValidNeighbors(const FHexAxialCoordinates& From,
                                       TArray<FHexAxialCoordinates>& Out) const
{
	Out.Reset();
	if (!GridRef) return;
	Out = GridRef->GetNeighbors(From);
}

int32 UHexPathFinder::Heuristic(const FHexAxialCoordinates& A, const FHexAxialCoordinates& B) const
{
	// Heuristique admissible fournie par la grille (doubled-q / axial)
	return GridRef ? GridRef->AxialDistance(A, B) : 0;
}

void UHexPathFinder::ReconstructPath(const TMap<FHexAxialCoordinates,FHexAxialCoordinates>& Parent,
                                     const FHexAxialCoordinates& Start,
                                     const FHexAxialCoordinates& Goal,
                                     TArray<FHexAxialCoordinates>& OutPath)
{
	OutPath.Reset();
	FHexAxialCoordinates Cur = Goal;
	OutPath.Add(Cur);
	while (!(Cur == Start))
	{
		if (const FHexAxialCoordinates* Prev = Parent.Find(Cur))
		{
			Cur = *Prev;
			OutPath.Add(Cur);
		}
		else
		{
			OutPath.Reset(); // cassé
			return;
		}
	}
	Algo::Reverse(OutPath);
}

TArray<FHexAxialCoordinates> UHexPathFinder::FindPath(const FHexAxialCoordinates& Start,
                                                      const FHexAxialCoordinates& Goal)
{
    TArray<FHexAxialCoordinates> Empty;
    if (!GridRef) return Empty;

    if (Start == Goal)
    {
        TArray<FHexAxialCoordinates> P; P.Add(Start);
        return P;
    }

    TSet<FHexAxialCoordinates> Open, Closed;
    TMap<FHexAxialCoordinates, FHexAxialCoordinates> Parent;
    TMap<FHexAxialCoordinates, int32> GScore, FScore;

    Open.Add(Start);
    Parent.Add(Start, Start);
    GScore.Add(Start, 0);
    FScore.Add(Start, Heuristic(Start, Goal));

    while (Open.Num() > 0)
    {
        // Choisir le noeud avec F-score min
        FHexAxialCoordinates Current = *Open.CreateConstIterator();
        int32 BestF = FScore.FindRef(Current);
        for (const FHexAxialCoordinates& N : Open)
        {
            const int32 FN = FScore.FindRef(N);
            if (FN < BestF)
            {
                BestF = FN;
                Current = N;
            }
        }

        if (Current == Goal)
        {
            TArray<FHexAxialCoordinates> OutPath;
            ReconstructPath(Parent, Start, Goal, OutPath);

            // Limite de déplacement par tour: 6 pas (donc 7 nœuds Start+6)
            if (OutPath.Num() > 0)
            {
                const int32 MaxStepsPerTurn = 6; // arbitraire pour l’instant
                const int32 MaxLen = FMath::Max(1, MaxStepsPerTurn + 1); // +1 pour inclure Start
                if (OutPath.Num() > MaxLen)
                {
                    OutPath.SetNum(MaxLen, /*bAllowShrinking*/ false);
                }
            }
            return OutPath;
        }

        Open.Remove(Current);
        Closed.Add(Current);

        // Voisins valides
        TArray<FHexAxialCoordinates> Neighbors;
        GetValidNeighbors(Current, Neighbors);

        const int32 GCur = GScore.FindRef(Current);
        for (const FHexAxialCoordinates& N : Neighbors)
        {
            if (Closed.Contains(N)) continue;

            const int32 TentativeG = GCur + 1;
            bool bBetter = false;

            if (!Open.Contains(N))
            {
                Open.Add(N);
                bBetter = true;
            }
            else if (TentativeG < GScore.FindRef(N))
            {
                bBetter = true;
            }

            if (bBetter)
            {
                Parent.Add(N, Current);
                GScore.Add(N, TentativeG);
                FScore.Add(N, TentativeG + Heuristic(N, Goal));
            }
        }
    }

    return Empty;
}