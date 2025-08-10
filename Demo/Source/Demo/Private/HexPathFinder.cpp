#include "HexPathFinder.h"
#include "HexGridManager.h"
#include "DemoGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/Reverse.h"

UHexPathFinder::UHexPathFinder()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// Directions déjà adaptées à ton axial (q peut varier de 2)
static const FHexAxialCoordinates GHexDirs[6] = {
    {+2,  0}, {+1, -1}, {-1, -1},
    {-2,  0}, {-1, +1}, {+1, +1}
};

void UHexPathFinder::GetValidNeighbors(UHexGridManager* Grid,
                                       const FHexAxialCoordinates& From,
                                       TArray<FHexAxialCoordinates>& OutNeighbors) const
{
    OutNeighbors.Reset();
    if (!Grid) return;
    OutNeighbors = Grid->GetNeighbors(From);
}

void UHexPathFinder::ReconstructPath(const TMap<FHexAxialCoordinates, FHexAxialCoordinates>& Parent,
                                     const FHexAxialCoordinates& Start,
                                     const FHexAxialCoordinates& Goal,
                                     TArray<FHexAxialCoordinates>& OutPath)
{
    OutPath.Reset();
    FHexAxialCoordinates Cur = Goal;
    OutPath.Add(Cur);

    while (!(Cur == Start))
    {
        const FHexAxialCoordinates* Prev = Parent.Find(Cur);
        if (!Prev) { OutPath.Reset(); return; }
        Cur = *Prev;
        OutPath.Add(Cur);
    }
    Algo::Reverse(OutPath);
}

TArray<FHexAxialCoordinates> UHexPathFinder::FindPath(const FHexAxialCoordinates& Start,
                                                      const FHexAxialCoordinates& Goal)
{
    TArray<FHexAxialCoordinates> Empty;

    UWorld* World = GetWorld();
    if (!World) return Empty;

    ADemoGameMode* GM = World->GetAuthGameMode<ADemoGameMode>();
    if (!GM) return Empty;

    UHexGridManager* Grid = GM->GetHexGridManager();
    if (!Grid) return Empty;

    if (Start == Goal)
    {
        TArray<FHexAxialCoordinates> P; P.Add(Start);
        return P;
    }

    // A*
    TSet<FHexAxialCoordinates> Open;
    TSet<FHexAxialCoordinates> Closed;
    TMap<FHexAxialCoordinates, FHexAxialCoordinates> Parent;
    TMap<FHexAxialCoordinates, int32> GScore;
    TMap<FHexAxialCoordinates, int32> FScore;

    Open.Add(Start);
    GScore.Add(Start, 0);
    FScore.Add(Start, Grid->AxialDistance(Start, Goal)); // heuristique admissible pour TON système

    TArray<FHexAxialCoordinates> Neigh;

    while (Open.Num() > 0)
    {
        // Sélection du node: min(F) puis tie-break sur G max (réduit les détours)
        FHexAxialCoordinates Current = *Open.CreateIterator();
        int32 BestF = *FScore.Find(Current);
        int32 BestG = *GScore.Find(Current);

        for (const FHexAxialCoordinates& N : Open)
        {
            const int32 FN = *FScore.Find(N);
            const int32 GN = *GScore.Find(N);
            if (FN < BestF || (FN == BestF && GN > BestG))
            {
                BestF = FN;
                BestG = GN;
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
        GetValidNeighbors(Grid, Current, Neigh);

        // G courant doit exister
        const int32* GcurPtr = GScore.Find(Current);
        if (!GcurPtr) { continue; }
        const int32 Gcur = *GcurPtr;

        for (const FHexAxialCoordinates& N : Neigh)
        {
            if (Closed.Contains(N)) continue;

            const int32 TentativeG = Gcur + 1; // coût uniforme

            bool bIsBetter = false;
            if (!Open.Contains(N))
            {
                Open.Add(N);
                bIsBetter = true;
            }
            else
            {
                if (const int32* Gold = GScore.Find(N))
                    bIsBetter = TentativeG < *Gold;
                else
                    bIsBetter = true; // en Open sans G, on met à jour
            }

            if (bIsBetter)
            {
                Parent.Add(N, Current);
                GScore.Add(N, TentativeG);
                FScore.Add(N, TentativeG + Grid->AxialDistance(N, Goal));
            }
        }
    }
    return Empty;
}
