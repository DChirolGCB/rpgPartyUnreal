// HexGridManager.cpp

#include "HexGridManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "HexTile.h"
#include "Kismet/GameplayStatics.h"

// Deltas voisins en doubled-q: W, NW, NE, E, SE, SW
static const FHexAxialCoordinates GDQ6[6] = {
    {-2, 0}, {-2, +1}, {0, +1}, {+2, 0}, {+2, -1}, {0, -1}};

static void DumpNeighborsOf(const UHexGridManager *Grid, const FHexAxialCoordinates &C, const TCHAR *Label)
{
    UE_LOG(LogTemp, Warning, TEXT("[Dbg] %s center=(%d,%d)"), Label, C.Q, C.R);

    // Candidats attendus
    for (int i = 0; i < 6; ++i)
    {
        const FHexAxialCoordinates N{C.Q + GDQ6[i].Q, C.R + GDQ6[i].R};
        const bool bPresent = (Grid && Grid->GetHexTileAt(N) != nullptr);
        UE_LOG(LogTemp, Warning, TEXT("[Dbg]  cand %d -> (%d,%d) present=%d"), i, N.Q, N.R, bPresent ? 1 : 0);
    }

    // Ce que renvoie réellement GetNeighbors
    if (Grid)
    {
        TArray<FHexAxialCoordinates> Out = Grid->GetNeighbors(C);
        FString S = FString::JoinBy(Out, TEXT(" "),
                                    [](const FHexAxialCoordinates &X)
                                    { return FString::Printf(TEXT("(%d,%d)"), X.Q, X.R); });
        UE_LOG(LogTemp, Warning, TEXT("[Dbg]  GetNeighbors -> %s"), *S);
    }
}

UHexGridManager::UHexGridManager()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHexGridManager::InitializeGrid(int32 Radius, TSubclassOf<AHexTile> TileClass)
{
    GridRadius = Radius;
    HexTileClass = TileClass;
    RebuildGrid();
}

void UHexGridManager::RebuildGrid()
{
    // 1) Détruire l'existant
    for (auto &Kvp : TilesMap)
        if (AHexTile *T = Kvp.Value)
            T->Destroy();
    TilesMap.Empty();

    // 2) Vérifs
    UWorld *World = GetWorld();
    if (!World || !*HexTileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildGrid: World or HexTileClass invalid"));
        return;
    }

    // 3) Origine par défaut
    if (GridOrigin.IsNearlyZero() && GetOwner())
        GridOrigin = GetOwner()->GetActorLocation();

    UE_LOG(LogTemp, Warning, TEXT("Rebuilding hex grid (Radius=%d, TileSize=%.1f)"), GridRadius, TileSize);

    // 4) Boucle de génération telle que tu l’utilises déjà (indices affichage Col/Row = Q/R)
    for (int32 q = -GridRadius; q <= GridRadius; ++q)
    {
        const int32 rMin = FMath::Max(-GridRadius, -q - GridRadius);
        const int32 rMax = FMath::Min(GridRadius, -q + GridRadius);

        for (int32 r = rMin; r <= rMax; ++r)
        {
            FVector SpawnLocation;
            if (!TryComputeTileSpawnPosition(q, r, SpawnLocation))
                continue;

            FActorSpawnParameters P;
            P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            AHexTile *Tile = World->SpawnActor<AHexTile>(HexTileClass, SpawnLocation, FRotator::ZeroRotator, P);
            if (!Tile)
                continue;

            const FHexAxialCoordinates Axial = MapSpawnIndexToAxial(q, r); // <- mapping corrigé
            Tile->SetAxialCoordinates(Axial);
#if WITH_EDITOR
            Tile->SetActorLabel(FString::Printf(TEXT("Hex (%d,%d)"), Axial.Q, Axial.R));
#endif
            TilesMap.Add(Axial, Tile);
        }
    }
    DumpNeighborsOf(this, FHexAxialCoordinates{0, 0}, TEXT("AfterRebuild"));
    DumpNeighborsOf(this, FHexAxialCoordinates{-8, -1}, TEXT("AfterRebuild"));

    // Optionnel
    BuildWorldNeighbors();
}

FVector UHexGridManager::ComputeTileSpawnPosition(int32 Q, int32 R) const
{
    // Placement monde EXISTANT conservé
    const float HexWidth = TileSize * 2.0f;
    const float HexHeight = TileSize * YSpacingFactor;
    const float XOffset = Q * HexWidth * XSpacingFactor;
    const float YOffset = R * HexHeight;

    const bool bIsOdd = bOffsetOnQ ? (Q & 1) : (R & 1);
    const float RowShift = bIsOdd ? (HexHeight * RowOffsetFactor) : 0.f;

    float FinalX = GridOrigin.X + XOffset + GlobalXYNudge.X;
    float FinalY = GridOrigin.Y + YOffset + RowShift + GlobalXYNudge.Y;
    float FinalZ = GridOrigin.Z;

    const FVector Base(FinalX, FinalY, FinalZ);

    // Trace vertical pour le Z
    const FVector Start = Base + FVector(0, 0, TraceHeight);
    const FVector End = Base - FVector(0, 0, TraceDepth);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HexGroundTrace), bTraceComplex);
    if (const AActor *Owner = GetOwner())
        Params.AddIgnoredActor(Owner);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    if (!bHit)
    {
        FCollisionObjectQueryParams Obj;
        Obj.AddObjectTypesToQuery(ECC_WorldStatic);
        Obj.AddObjectTypesToQuery(ECC_WorldDynamic);
        bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, Obj, Params);
    }
    /**
    #if WITH_EDITOR
        if (bDebugTrace)
        {
            DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 5.f, 0, 2.f);
            if (bHit) DrawDebugPoint(GetWorld(), Hit.Location, 12.f, FColor::Yellow, false, 5.f);
        }
    #endif
    */
    if (bHit)
        FinalZ = Hit.Location.Z + TileZOffset;
    return FVector(FinalX, FinalY, FinalZ);
}

bool UHexGridManager::TryComputeTileSpawnPosition(int32 Q, int32 R, FVector &OutLocation) const
{
    // Placement monde EXISTANT conservé
    const float HexWidth = TileSize * 2.0f;
    const float HexHeight = TileSize * YSpacingFactor;
    const float XOffset = Q * HexWidth * XSpacingFactor;
    const float YOffset = R * HexHeight;

    const bool bIsOdd = bOffsetOnQ ? (Q & 1) : (R & 1);
    const float RowShift = bIsOdd ? (HexHeight * RowOffsetFactor) : 0.f;

    float FinalX = GridOrigin.X + XOffset + GlobalXYNudge.X;
    float FinalY = GridOrigin.Y + YOffset + RowShift + GlobalXYNudge.Y;
    float FinalZ = GridOrigin.Z;

    const FVector Base(FinalX, FinalY, FinalZ);
    const FVector Start = Base + FVector(0, 0, TraceHeight);
    const FVector End = Base - FVector(0, 0, TraceDepth);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HexGroundTrace), bTraceComplex);
    if (const AActor *Owner = GetOwner())
        Params.AddIgnoredActor(Owner);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    if (!bHit)
    {
        FCollisionObjectQueryParams Obj;
        Obj.AddObjectTypesToQuery(ECC_WorldStatic);
        Obj.AddObjectTypesToQuery(ECC_WorldDynamic);
        bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, Obj, Params);
    }
/* 
#if WITH_EDITOR
    if (bDebugTrace)
    {
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 5.f, 0, 2.f);
        if (bHit)
            DrawDebugPoint(GetWorld(), Hit.Location, 12.f, FColor::Yellow, false, 5.f);
    }
#endif
 */
    if (!bHit)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[HexGrid] (%d,%d): no ground hit"), Q, R);
        return false;
    }

    const AActor *HitA = Hit.GetActor();
    if (bSkipTilesOverFloor)
    {
        if (!HitA)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HexGrid] (%d,%d): hit no actor -> skip"), Q, R);
            return false;
        }
        if (HitA->ActorHasTag(FloorTag))
        {
            UE_LOG(LogTemp, Warning, TEXT("[HexGrid] (%d,%d): Floor tag on %s -> skip"), Q, R, *HitA->GetName());
            return false;
        }
    }

    FinalZ = Hit.Location.Z + TileZOffset;
    OutLocation = FVector(FinalX, FinalY, FinalZ);
    return true;
}

// -------- Mapping Offset -> Axial standard -> Doubled-Q --------

static FORCEINLINE int32 FloorDiv2_Int(int32 x)
{
    // floor(x/2) pour entiers signés
    return (x >= 0) ? (x >> 1) : -((-x + 1) >> 1);
}

FHexAxialCoordinates UHexGridManager::MapSpawnIndexToAxial(int32 Col, int32 Row) const
{
    // Col/Row = indices utilisés par TON placement existant
    // Convertit Offset → axial standard, puis axial → doubled-q

    int32 q_ax = 0;
    int32 r_ax = 0;

    if (bOffsetOnQ)
    {
        // Colonnes décalées (pointy-top). Odd-Q
        // axial: q = col ; r = row - floor(col/2)
        q_ax = Col;
        r_ax = Row - FloorDiv2_Int(Col);
    }
    else
    {
        // Lignes décalées (flat-top). Odd-R
        // axial: q = col - floor(row/2) ; r = row
        q_ax = Col - FloorDiv2_Int(Row);
        r_ax = Row;
    }

    // Doubled-Q: q' = 2*q ; r' = r
    return FHexAxialCoordinates{q_ax * 2, r_ax};
}

// ---------------------------------------------------------------

AHexTile *UHexGridManager::GetHexTileAt(const FHexAxialCoordinates &Coords) const
{
    if (AHexTile *const *Found = TilesMap.Find(Coords))
        return *Found;
    return nullptr;
}

TArray<FHexAxialCoordinates> UHexGridManager::GetNeighbors(const FHexAxialCoordinates &C) const
{
    TArray<FHexAxialCoordinates> Out;
    Out.Reserve(6);
    for (int i = 0; i < 6; ++i)
    {
        const FHexAxialCoordinates N{C.Q + GDQ6[i].Q, C.R + GDQ6[i].R};
        if (GetHexTileAt(N))
            Out.Add(N);

        // Log ciblé sur la tuile problématique
        if (C.Q == -8 && C.R == -1)
        {
            const bool bHas = GetHexTileAt(N) != nullptr;
            UE_LOG(LogTemp, Warning, TEXT("[Dbg] GetNeighbors@(-8,-1) try (%d,%d) -> %d"), N.Q, N.R, bHas ? 1 : 0);
        }
    }
    return Out;
}

int32 UHexGridManager::AxialDistance(const FHexAxialCoordinates &A, const FHexAxialCoordinates &B) const
{
    const int32 dq = FMath::Abs(A.Q - B.Q);
    const int32 dr = FMath::Abs(A.R - B.R);
    return FMath::Max(dr, (dq + dr) / 2);
}

void UHexGridManager::BuildWorldNeighbors()
{
    WorldNeighbors.Empty();

    UWorld *World = GetWorld();
    if (!World)
        return;

    TMap<FHexAxialCoordinates, FVector> Pos;
    for (TActorIterator<AHexTile> It(World); It; ++It)
        if (IsValid(*It))
            Pos.Add(It->GetAxialCoordinates(), It->GetActorLocation());

    for (const auto &ItA : Pos)
    {
        const FHexAxialCoordinates AKey = ItA.Key;
        const FVector APos = ItA.Value;

        struct FNeighborDist
        {
            FHexAxialCoordinates Key;
            float D2;
        };
        TArray<FNeighborDist> D;
        D.Reserve(16);

        for (const auto &ItB : Pos)
        {
            if (ItB.Key == AKey)
                continue;
            const float d2 = (ItB.Value - APos).SizeSquared2D(); // float sûr
            D.Add({ItB.Key, d2});
        }

        D.Sort([](const FNeighborDist &L, const FNeighborDist &R)
               { return L.D2 < R.D2; });

        TArray<FHexAxialCoordinates> Neigh;
        const int32 Count = FMath::Min(6, D.Num());
        Neigh.Reserve(Count);
        for (int32 i = 0; i < Count; ++i)
            Neigh.Add(D[i].Key);

        WorldNeighbors.Add(AKey, MoveTemp(Neigh));
    }

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Warning, TEXT("[Hex] WorldNeighbors built for %d tiles"), WorldNeighbors.Num());
#endif
}

void UHexGridManager::GetNeighborsByWorld(const FHexAxialCoordinates &From, TArray<FHexAxialCoordinates> &Out) const
{
    if (const TArray<FHexAxialCoordinates> *Found = WorldNeighbors.Find(From))
        Out = *Found;
    else
        Out.Reset();
}
