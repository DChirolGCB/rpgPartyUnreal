// HexGridManager.cpp

#include "HexGridManager.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "DrawDebugHelpers.h"
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
    for (auto& Kvp : TilesMap)
    {
        if (AHexTile* T = Kvp.Value)
        {
            T->Destroy();
        }
    }
    TilesMap.Empty();

    // 2) Vérifs
    UWorld* World = GetWorld();
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
        const int32 rMax = FMath::Min( GridRadius, -q + GridRadius);

        for (int32 r = rMin; r <= rMax; ++r)
        {
            const FVector SpawnLocation = ComputeTileSpawnPosition(q, r);

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
            TilesMap.Add(Axial, Tile);
        }
    }
}


FVector UHexGridManager::ComputeTileSpawnPosition(int32 Q, int32 R) const
{
    // Dimensions dérivées
    const float HexWidth  = TileSize * 2.0f;
    const float HexHeight = TileSize * YSpacingFactor; // ~ 0.866 * TileSize

    // Espacement XY via facteurs éditables
    const float XOffset = Q * HexWidth  * XSpacingFactor;
    const float YOffset = R * HexHeight;

    // Décalage demi-ligne : sur Q impair (ou R si bOffsetOnQ=false)
    const bool  bIsOdd   = bOffsetOnQ ? (Q & 1) : (R & 1);
    const float RowShift = bIsOdd ? (HexHeight * RowOffsetFactor) : 0.f;

    float FinalX = GridOrigin.X + XOffset + GlobalXYNudge.X;
    float FinalY = GridOrigin.Y + YOffset + RowShift + GlobalXYNudge.Y;
    float FinalZ = GridOrigin.Z; // ajusté par le trace

    const FVector BasePosition(FinalX, FinalY, FinalZ);

    // ---- Z : line trace vertical (comme ta version qui marchait) ----
    const FVector TraceStart = BasePosition + FVector(0.f, 0.f,  TraceHeight);
    const FVector TraceEnd   = BasePosition + FVector(0.f, 0.f, -TraceDepth);

    FHitResult HitResult;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HexGroundTrace), bTraceComplex);
    if (const AActor* Owner = GetOwner())
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

AHexTile* UHexGridManager::GetHexTileAt(const FHexAxialCoordinates& Coords) const
{
    if (AHexTile* const* Found = TilesMap.Find(Coords))
    {
        return *Found;
    }
    return nullptr;
}

TArray<FHexAxialCoordinates> UHexGridManager::GetNeighbors(const FHexAxialCoordinates& Coords) const
{
    // Directions axiales standard
    static const FHexAxialCoordinates Directions[6] =
    {
        {+1,  0},
        {+1, -1},
        { 0, -1},
        {-1,  0},
        {-1, +1},
        { 0, +1}
    };

    TArray<FHexAxialCoordinates> Out;
    Out.Reserve(6);

    for (const FHexAxialCoordinates& Dir : Directions)
    {
        const FHexAxialCoordinates N(Coords.Q + Dir.Q, Coords.R + Dir.R);
        if (TilesMap.Contains(N))
        {
            Out.Add(N);
        }
    }
    return Out;
}
