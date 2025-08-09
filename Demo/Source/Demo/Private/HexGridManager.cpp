// HexGridManager.cpp

#include "HexGridManager.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "DrawDebugHelpers.h"
#include "Algo/Sort.h"
#include "HexTile.h"
#include "Algo/Sort.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "HexTile.h"
#include "Kismet/GameplayStatics.h"

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
    {
        if (AHexTile *T = Kvp.Value)
        {
            T->Destroy();
        }
    }
    TilesMap.Empty();

    // 2) Vérifs
    UWorld *World = GetWorld();
    if (!World || !*HexTileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildGrid: World or HexTileClass invalid"));
        return;
    }

    // 3) Origine par défaut = Owner si GridOrigin est 0,0,0
    if (GridOrigin.IsNearlyZero() && GetOwner())
    {
        GridOrigin = GetOwner()->GetActorLocation();
    }

    UE_LOG(LogTemp, Warning, TEXT("Rebuilding hex grid (Radius=%d, TileSize=%.1f)"), GridRadius, TileSize);

    // 4) Boucle de génération (copie/colle ta boucle existante)
     for (int32 q = -GridRadius; q <= GridRadius; ++q)
    {
        const int32 rMin = FMath::Max(-GridRadius, -q - GridRadius);
        const int32 rMax = FMath::Min(GridRadius, -q + GridRadius);

        for (int32 r = rMin; r <= rMax; ++r)
        {
            FVector SpawnLocation;
            if (!TryComputeTileSpawnPosition(q, r, SpawnLocation))
            {
                // filtré (océan) => on saute
                continue;
            }

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            AHexTile* Tile = World->SpawnActor<AHexTile>(HexTileClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            if (!Tile)
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn HexTile at (%d,%d)"), q, r);
                continue;
            }

            const FHexAxialCoordinates Axial{ q, r };
            Tile->SetAxialCoordinates(Axial);
            // Label lisible dans l'outliner pour debug
#if WITH_EDITOR
            Tile->SetActorLabel(FString::Printf(TEXT("Hex (%d,%d)"), q, r));
#endif
            TilesMap.Add(Axial, Tile);
        }
    }
    // Optionnel: calcule des voisins par proximité XY pour debug/visualisation
    BuildWorldNeighbors();
}

FVector UHexGridManager::ComputeTileSpawnPosition(int32 Q, int32 R) const
{
    // Dimensions dérivées
    const float HexWidth = TileSize * 2.0f;
    const float HexHeight = TileSize * YSpacingFactor; // ~ 0.866 * TileSize

    // Espacement XY via facteurs éditables
    const float XOffset = Q * HexWidth * XSpacingFactor;
    const float YOffset = R * HexHeight;

    // Décalage demi-ligne : sur Q impair (ou R si bOffsetOnQ=false)
    const bool bIsOdd = bOffsetOnQ ? (Q & 1) : (R & 1);
    const float RowShift = bIsOdd ? (HexHeight * RowOffsetFactor) : 0.f;

    float FinalX = GridOrigin.X + XOffset + GlobalXYNudge.X;
    float FinalY = GridOrigin.Y + YOffset + RowShift + GlobalXYNudge.Y;
    float FinalZ = GridOrigin.Z; // ajusté par le trace

    const FVector BasePosition(FinalX, FinalY, FinalZ);

    // ---- Z : line trace vertical (comme ta version qui marchait) ----
    const FVector TraceStart = BasePosition + FVector(0.f, 0.f, TraceHeight);
    const FVector TraceEnd = BasePosition + FVector(0.f, 0.f, -TraceDepth);

    FHitResult HitResult;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HexGroundTrace), bTraceComplex);
    if (const AActor *Owner = GetOwner())
    {
        Params.AddIgnoredActor(Owner);
    }

    bool bHit = false;

    // 1) Visibility d’abord (ta version marchait ainsi)
    bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, Params);

    // 2) Fallback Static/Dynamic si nécessaire (au cas où Visibility ne serait pas bloqué)
    if (!bHit)
    {
        FCollisionObjectQueryParams ObjParams;
        ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
        ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
        bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, ObjParams, Params);
    }

#if WITH_EDITOR
    if (bDebugTrace)
    {
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 5.0f, 0, 2.0f);
        if (bHit)
        {
            DrawDebugPoint(GetWorld(), HitResult.Location, 12.0f, FColor::Yellow, false, 5.0f);
        }
    }
#endif

    if (bHit)
    {
        FinalZ = HitResult.Location.Z + TileZOffset;
    }

    return FVector(FinalX, FinalY, FinalZ);
}

bool UHexGridManager::TryComputeTileSpawnPosition(int32 Q, int32 R, FVector& OutLocation) const
{
    // === recalc XY identique à ComputeTileSpawnPosition ===
    const float HexWidth  = TileSize * 2.0f;
    const float HexHeight = TileSize * YSpacingFactor;

    const float XOffset = Q * HexWidth * XSpacingFactor;
    const float YOffset = R * HexHeight;

    const bool bIsOdd = bOffsetOnQ ? (Q & 1) : (R & 1);
    const float RowShift = bIsOdd ? (HexHeight * RowOffsetFactor) : 0.f;

    float FinalX = GridOrigin.X + XOffset + GlobalXYNudge.X;
    float FinalY = GridOrigin.Y + YOffset + RowShift + GlobalXYNudge.Y;
    float FinalZ = GridOrigin.Z;

    const FVector Base(FinalX, FinalY, FinalZ);

    // === trace vertical (Visibility puis fallback Static/Dynamic) ===
    const FVector Start = Base + FVector(0,0, TraceHeight);
    const FVector End   = Base - FVector(0,0, TraceDepth);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HexGroundTrace), bTraceComplex);
    if (const AActor* Owner = GetOwner()) Params.AddIgnoredActor(Owner);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    if (!bHit)
    {
        FCollisionObjectQueryParams Obj;
        Obj.AddObjectTypesToQuery(ECC_WorldStatic);
        Obj.AddObjectTypesToQuery(ECC_WorldDynamic);
        bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, Obj, Params);
    }

#if WITH_EDITOR
    if (bDebugTrace)
    {
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 5.f, 0, 2.f);
        if (bHit) DrawDebugPoint(GetWorld(), Hit.Location, 12.f, FColor::Yellow, false, 5.f);
    }
#endif


    if (!bHit)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[HexGrid] (%d,%d) : Aucun hit, pas de tuile."), Q, R);
        return false;
    }

    // Log détaillé du hit
    const AActor* HitA = Hit.GetActor();
    FString TagList;
    if (HitA)
    {
        for (const FName& Tag : HitA->Tags)
        {
            TagList += Tag.ToString() + TEXT(", ");
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[HexGrid] (%d,%d) : Hit Z=%.2f, Actor=%s, Tags=[%s]"), Q, R, Hit.Location.Z, HitA ? *HitA->GetName() : TEXT("(null)"), *TagList);

    // Empêche la création de tuile si Z == 1
    if (FMath::IsNearlyEqual(Hit.Location.Z, 1.0f, KINDA_SMALL_NUMBER))
    {
        UE_LOG(LogTemp, Warning, TEXT("[HexGrid] (%d,%d) : Hit Z=1, tuile ignorée."), Q, R);
        return false;
    }

    // Si premier hit == océan -> on rejette (uniquement par tag)
    if (bSkipTilesOverFloor)
    {
        if (HitA)
        {
            if (HitA->ActorHasTag(FloorTag))
            {
                UE_LOG(LogTemp, Warning, TEXT("[HexGrid] (%d,%d) : Hit Floor (tag présent sur %s), tuile ignorée."), Q, R, *HitA->GetName());
                return false;
            }
            else
            {
                UE_LOG(LogTemp, Verbose, TEXT("[HexGrid] (%d,%d) : Hit acteur (%s) sans tag Floor, tuile créée."), Q, R, *HitA->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HexGrid] (%d,%d) : Hit sans acteur, tuile ignorée."), Q, R);
        }
    }

    FinalZ = Hit.Location.Z + TileZOffset;
    OutLocation = FVector(FinalX, FinalY, FinalZ);
    return true;
}


AHexTile *UHexGridManager::GetHexTileAt(const FHexAxialCoordinates &Coords) const
{
    if (AHexTile *const *Found = TilesMap.Find(Coords))
    {
        return *Found;
    }
    return nullptr;
}

TArray<FHexAxialCoordinates> UHexGridManager::GetNeighbors(const FHexAxialCoordinates &Coords) const
{
    // Directions axiales standard
    static const FHexAxialCoordinates Directions[6] =
        {
            {+1, 0},
            {+1, -1},
            {0, -1},
            {-1, 0},
            {-1, +1},
            {0, +1}};

    TArray<FHexAxialCoordinates> Out;
    Out.Reserve(6);

    for (const FHexAxialCoordinates &Dir : Directions)
    {
        const FHexAxialCoordinates N(Coords.Q + Dir.Q, Coords.R + Dir.R);
        if (TilesMap.Contains(N))
        {
            Out.Add(N);
        }
    }
    return Out;
}

void UHexGridManager::BuildWorldNeighbors()
{
    WorldNeighbors.Empty();

    UWorld *World = GetWorld();
    if (!World)
        return;

    // 1) Récupère positions XY de toutes les tuiles présentes
    TMap<FHexAxialCoordinates, FVector> Pos;
    for (TActorIterator<AHexTile> It(World); It; ++It)
    {
        AHexTile *T = *It;
        if (!IsValid(T))
            continue;
        Pos.Add(T->GetAxialCoordinates(), T->GetActorLocation());
    }

    // 2) Pour chaque tuile, prend les 6 plus proches en XY
    for (const auto &It : Pos)
    {
        const FHexAxialCoordinates AKey = It.Key;
        const FVector APos = It.Value;

        struct FNeighborDist
        {
            FHexAxialCoordinates Key;
            float Dist2;
        };
        TArray<FNeighborDist> Dists;
        Dists.Reserve(16);

        for (const auto &Jt : Pos)
        {
            if (Jt.Key == AKey)
                continue;
            const FVector BPos = Jt.Value;
            const float DX = BPos.X - APos.X;
            const float DY = BPos.Y - APos.Y;
            Dists.Add({Jt.Key, DX * DX + DY * DY});
        }

        Dists.Sort([](const FNeighborDist &L, const FNeighborDist &R)
                   { return L.Dist2 < R.Dist2; });

        TArray<FHexAxialCoordinates> Neigh;
        const int32 Count = FMath::Min(6, Dists.Num());
        Neigh.Reserve(Count);
        for (int32 i = 0; i < Count; ++i)
        {
            Neigh.Add(Dists[i].Key);
        }

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
