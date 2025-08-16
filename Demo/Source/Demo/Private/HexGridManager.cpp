// HexGridManager.cpp
#include "HexGridManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HexTile.h"
#include "Kismet/GameplayStatics.h"

// Doubled-q axial neighbor deltas: W, NW, NE, E, SE, SW
static const FHexAxialCoordinates GDQ6[6] = {
    {-2, 0}, {-2, +1}, {0, +1}, {+2, 0}, {+2, -1}, {0, -1}};

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
    // Destroy existing tiles
    for (auto &Kvp : TilesMap)
    {
        if (AHexTile *T = Kvp.Value.Get())
            if (IsValid(T) && !T->IsActorBeingDestroyed())
                T->Destroy();
    }
    TilesMap.Empty();

    UWorld *World = GetWorld();
    if (!World || !*HexTileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[HexGrid] RebuildGrid: World or HexTileClass invalid"));
        return;
    }

    // Set default origin if not set
    if (GridOrigin.IsNearlyZero() && GetOwner())
    {
        GridOrigin = GetOwner()->GetActorLocation();
    }

    UE_LOG(LogTemp, Warning, TEXT("[HexGrid] Rebuild: Radius=%d, TileSize=%.1f"), GridRadius, TileSize);

    // Generate hex tiles (axial ring by ring)
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

            const FHexAxialCoordinates Axial = MapSpawnIndexToAxial(q, r);
            Tile->SetAxialCoordinates(Axial);
#if WITH_EDITOR
            Tile->SetActorLabel(FString::Printf(TEXT("Hex (%d,%d)"), Axial.Q, Axial.R));
#endif
            TilesMap.Add(Axial, TWeakObjectPtr<AHexTile>(Tile));
        }
    }

    ApplySpecialTiles();
    BuildWorldNeighbors();
}

void UHexGridManager::ApplySpecialTiles()
{
    for (const FHexAxialCoordinates &C : ShopTiles)
    {
        if (AHexTile *T = GetHexTileAt(C))
        {
            T->SetTileType(EHexTileType::Shop);
#if WITH_EDITOR
            T->SetActorLabel(FString::Printf(TEXT("Shop (%d,%d)"), C.Q, C.R));
#endif
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HexGrid] Unknown shop coord (%d,%d)"), C.Q, C.R);
        }
    }
}

FVector UHexGridManager::ComputeTileSpawnPosition(int32 Q, int32 R) const
{
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

    if (bHit)
    {
        FinalZ = Hit.Location.Z + TileZOffset;
    }

    return FVector(FinalX, FinalY, FinalZ);
}

bool UHexGridManager::TryComputeTileSpawnPosition(int32 Q, int32 R, FVector &OutLocation) const
{
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

// floor(x/2) for signed ints
static FORCEINLINE int32 FloorDiv2_Int(int32 x)
{
    return (x >= 0) ? (x >> 1) : -((-x + 1) >> 1);
}

FHexAxialCoordinates UHexGridManager::MapSpawnIndexToAxial(int32 Col, int32 Row) const
{
    int32 q_ax = 0;
    int32 r_ax = 0;

    if (bOffsetOnQ)
    {
        // Odd-Q (pointy-top): q=col; r=row - floor(col/2)
        q_ax = Col;
        r_ax = Row - FloorDiv2_Int(Col);
    }
    else
    {
        // Odd-R (flat-top): q=col - floor(row/2); r=row
        q_ax = Col - FloorDiv2_Int(Row);
        r_ax = Row;
    }

    // Optional label tweaks (invert/swap); affects labels only
    if (bInvertQAxisForLabels)
        q_ax = -q_ax;
    if (bInvertRAxisForLabels)
        r_ax = -r_ax;
    if (bSwapQRForLabels)
        Swap(q_ax, r_ax);

    // Doubled-q labeling if requested
    if (bUseDoubledQForLabels)
    {
        return FHexAxialCoordinates{q_ax * 2, r_ax};
    }
    return FHexAxialCoordinates{q_ax, r_ax};
}

AHexTile *UHexGridManager::GetHexTileAt(const FHexAxialCoordinates &Coords) const
{
    if (const TWeakObjectPtr<AHexTile> *Found = TilesMap.Find(Coords))
        return Found->IsValid() ? Found->Get() : nullptr;
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
    }
    return Out;
}

int32 UHexGridManager::AxialDistance(const FHexAxialCoordinates &A,
                                     const FHexAxialCoordinates &B) const
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
            const float d2 = (ItB.Value - APos).SizeSquared2D();
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
    UE_LOG(LogTemp, Warning, TEXT("[HexGrid] WorldNeighbors built for %d tiles"), WorldNeighbors.Num());
#endif
}

void UHexGridManager::GetNeighborsByWorld(const FHexAxialCoordinates &From,
                                          TArray<FHexAxialCoordinates> &Out) const
{
    if (const TArray<FHexAxialCoordinates> *Found = WorldNeighbors.Find(From))
        Out = *Found;
    else
        Out.Reset();
}

void UHexGridManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    TilesMap.Empty();
    WorldNeighbors.Empty();
    Super::EndPlay(EndPlayReason);
}

void UHexGridManager::BeginDestroy()
{
    TilesMap.Empty();
    WorldNeighbors.Empty();
    Super::BeginDestroy();
}
