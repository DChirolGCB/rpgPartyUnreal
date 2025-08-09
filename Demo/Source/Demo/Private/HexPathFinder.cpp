#include "HexPathFinder.h"
#include "HexGridManager.h"
#include "DemoGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/Reverse.h"

UHexPathFinder::UHexPathFinder()
{
    PrimaryComponentTick.bCanEverTick = false;
}

static const FHexAxialCoordinates GHexDirs[6] = {
    {+2,  0}, {+1, -1}, {-1, -1},
    {-2,  0}, {-1, +1}, {+1, +1}
};

int32 UHexPathFinder::HexDistance(const FHexAxialCoordinates& A, const FHexAxialCoordinates& B)
{
    const int32 dq = A.Q - B.Q;
    const int32 dr = A.R - B.R;
    const int32 ds = (-(A.Q + A.R)) - (-(B.Q + B.R)); // s = -q - r
    return (FMath::Abs(dq) + FMath::Abs(dr) + FMath::Abs(ds)) / 2;
}

void UHexPathFinder::GetValidNeighbors(UHexGridManager* Grid,
                                       const FHexAxialCoordinates& From,
                                       TArray<FHexAxialCoordinates>& OutNeighbors) const
{
    OutNeighbors.Reset();
    if (!Grid) return;

    // Utilise l'adjacence axiale standard, limitée aux tuiles réellement présentes
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
        if (!Prev) { OutPath.Reset(); return; } // pas de chemin
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
    FScore.Add(Start, HexDistance(Start, Goal));

    TArray<FHexAxialCoordinates> Neigh;

    while (Open.Num() > 0)
    {
        // Cherche le node de Open avec le plus petit F
        FHexAxialCoordinates Current = *Open.CreateIterator();
        int32 BestF = FScore.FindRef(Current);
        for (const FHexAxialCoordinates& N : Open)
        {
            const int32 F = FScore.FindRef(N);
            if (F < BestF)
            {
                BestF = F;
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

        GetValidNeighbors(Grid, Current, Neigh);
        for (const FHexAxialCoordinates& N : Neigh)
        {
            if (Closed.Contains(N)) continue;

            const int32 TentativeG = GScore.FindRef(Current) + 1; // coût uniforme par step

            bool bIsBetter = false;
            if (!Open.Contains(N))
            {
                Open.Add(N);
                bIsBetter = true;
            }
            else if (TentativeG < GScore.FindRef(N))
            {
                bIsBetter = true;
            }

            if (bIsBetter)
            {
                Parent.Add(N, Current);
                GScore.Add(N, TentativeG);
                FScore.Add(N, TentativeG + HexDistance(N, Goal));
            }
        }
    }

    // Pas de chemin
    return Empty;
}
