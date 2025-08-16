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

    if (Start == Goal) { TArray<FHexAxialCoordinates> P; P.Add(Start); return P; }

    auto Heuristic = [this](const FHexAxialCoordinates& A, const FHexAxialCoordinates& B)
    {
        return GridRef->AxialDistance(A, B);
    };

    TSet<FHexAxialCoordinates> Open, Closed;
    TMap<FHexAxialCoordinates,FHexAxialCoordinates> Parent;
    TMap<FHexAxialCoordinates,int32> GScore, FScore;

    Open.Add(Start);
    GScore.Add(Start, 0);
    FScore.Add(Start, Heuristic(Start, Goal));

    TArray<FHexAxialCoordinates> Neigh;

    while (Open.Num() > 0)
    {
        // pick best by F asc then G desc
        FHexAxialCoordinates Current = *Open.CreateIterator();
        int32 BestF = *FScore.Find(Current);
        int32 BestG = *GScore.Find(Current);
        for (const FHexAxialCoordinates& N : Open)
        {
            const int32* FN = FScore.Find(N);
            const int32* GN = GScore.Find(N);
            if (!FN || !GN) continue;
            if (*FN < BestF || (*FN == BestF && *GN > BestG)) { BestF = *FN; BestG = *GN; Current = N; }
        }

        if (Current == Goal)
        {
            TArray<FHexAxialCoordinates> Path;
            ReconstructPath(Parent, Start, Goal, Path);
            return Path;
        }

        Open.Remove(Current);
        Closed.Add(Current);

        Neigh.Reset();
        GetValidNeighbors(Current, Neigh);  // <-- correct ici

        const int32* GcurPtr = GScore.Find(Current);
        if (!GcurPtr) continue;
        const int32 Gcur = *GcurPtr;

        for (const FHexAxialCoordinates& N : Neigh)
        {
            if (Closed.Contains(N)) continue;

            const int32 TentativeG = Gcur + 1;
            bool bBetter = false;

            if (!Open.Contains(N)) { Open.Add(N); bBetter = true; }
            else if (const int32* Gold = GScore.Find(N)) { bBetter = TentativeG < *Gold; }
            else { bBetter = true; }

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
