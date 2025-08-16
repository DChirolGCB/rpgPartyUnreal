// HexPathFinder.cpp
#include "HexPathFinder.h"
#include "HexGridManager.h"
#include "Algo/Reverse.h"

UHexPathFinder::UHexPathFinder()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHexPathFinder::GetValidNeighbors(const FHexAxialCoordinates &From,
									   TArray<FHexAxialCoordinates> &Out) const
{
	Out.Reset();
	if (!GridRef)
		return;
	Out = GridRef->GetNeighbors(From);
}

int32 UHexPathFinder::Heuristic(const FHexAxialCoordinates &A,
								const FHexAxialCoordinates &B) const
{
	return GridRef ? GridRef->AxialDistance(A, B) : 0;
}

void UHexPathFinder::ReconstructPath(const TMap<FHexAxialCoordinates, FHexAxialCoordinates> &Parent,
									 const FHexAxialCoordinates &Start,
									 const FHexAxialCoordinates &Goal,
									 TArray<FHexAxialCoordinates> &OutPath)
{
	OutPath.Reset();
	FHexAxialCoordinates Cur = Goal;
	OutPath.Add(Cur);

	while (!(Cur == Start))
	{
		if (const FHexAxialCoordinates *Prev = Parent.Find(Cur))
		{
			Cur = *Prev;
			OutPath.Add(Cur);
		}
		else
		{
			OutPath.Reset(); // broken chain
			return;
		}
	}

	Algo::Reverse(OutPath);
}

TArray<FHexAxialCoordinates> UHexPathFinder::FindPath(const FHexAxialCoordinates &Start,
													  const FHexAxialCoordinates &Goal)
{
	TArray<FHexAxialCoordinates> Empty;
	if (!GridRef)
		return Empty;

	if (Start == Goal)
	{
		TArray<FHexAxialCoordinates> Single;
		Single.Add(Start);
		return Single;
	}

	auto H = [this](const FHexAxialCoordinates &A, const FHexAxialCoordinates &B)
	{
		return GridRef->AxialDistance(A, B);
	};

	TSet<FHexAxialCoordinates> Open, Closed;
	TMap<FHexAxialCoordinates, FHexAxialCoordinates> Parent;
	TMap<FHexAxialCoordinates, int32> GScore, FScore;

	Open.Add(Start);
	GScore.Add(Start, 0);
	FScore.Add(Start, H(Start, Goal));

	TArray<FHexAxialCoordinates> Neigh;

	while (Open.Num() > 0)
	{
		// Pick best by F ascending, then G descending
		FHexAxialCoordinates Current = *Open.CreateIterator();

		const int32 *FCurPtr = FScore.Find(Current);
		const int32 *GCurPtr = GScore.Find(Current);
		if (!FCurPtr || !GCurPtr)
		{
			Open.Remove(Current);
			continue;
		}

		int32 BestF = *FCurPtr;
		int32 BestG = *GCurPtr;

		for (const FHexAxialCoordinates &N : Open)
		{
			const int32 *FN = FScore.Find(N);
			const int32 *GN = GScore.Find(N);
			if (!FN || !GN)
				continue;

			const bool bBetter =
				(*FN < BestF) || ((*FN == BestF) && (*GN > BestG));
			if (bBetter)
			{
				BestF = *FN;
				BestG = *GN;
				Current = N;
			}
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
		GetValidNeighbors(Current, Neigh);

		const int32 *GCur = GScore.Find(Current);
		if (!GCur)
			continue;

		for (const FHexAxialCoordinates &N : Neigh)
		{
			if (Closed.Contains(N))
				continue;

			const int32 TentativeG = *GCur + 1;
			bool bBetter = false;

			if (!Open.Contains(N))
			{
				Open.Add(N);
				bBetter = true;
			}
			else if (const int32 *Gold = GScore.Find(N))
			{
				bBetter = TentativeG < *Gold;
			}
			else
			{
				bBetter = true;
			}

			if (bBetter)
			{
				Parent.Add(N, Current);
				GScore.Add(N, TentativeG);
				FScore.Add(N, TentativeG + H(N, Goal));
			}
		}
	}

	return Empty;
}
