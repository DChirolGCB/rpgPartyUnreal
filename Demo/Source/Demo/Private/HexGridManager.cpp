#include "HexGridManager.h"
#include "HexTile.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

UHexGridManager::UHexGridManager()
{
    PrimaryComponentTick.bCanEverTick = false;
}

const TArray<FHexAxialCoordinates> UHexGridManager::HexDirections = {
    FHexAxialCoordinates(2, 0),   // Droite même ligne
    FHexAxialCoordinates(-2, 0),  // Gauche même ligne  
    FHexAxialCoordinates(1, 1),   // Diagonale droite bas
    FHexAxialCoordinates(-1, 1),  // Diagonale gauche bas
    FHexAxialCoordinates(1, -1),  // Diagonale droite haut
    FHexAxialCoordinates(-1, -1)  // Diagonale gauche haut
};


TArray<FHexAxialCoordinates> UHexGridManager::GetNeighbors(const FHexAxialCoordinates& Coords)
{
    TArray<FHexAxialCoordinates> Neighbors;

    for (const auto& Direction : HexDirections)
    {
        FHexAxialCoordinates NeighborCoord(Coords.Q + Direction.Q, Coords.R + Direction.R);

        if (HexTiles.Contains(NeighborCoord))
        {
            Neighbors.Add(NeighborCoord);
        }
    }

    return Neighbors;
}

void UHexGridManager::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("HexGridManager BeginPlay - TEST LOG"));
}

void UHexGridManager::InitializeGrid(int32 InRadius, TSubclassOf<AHexTile> InHexTileClass)
{
    GridRadius = InRadius;
    HexTileClass = InHexTileClass;

    UE_LOG(LogTemp, Warning, TEXT("HexGridManager initialized with size %dx%d"), GridRadius, GridRadius);
}

void UHexGridManager::GenerateGrid()
{
    GridOrigin = FVector(-150.f, -1100.f, 0.f);
    if (!HexTileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid hex tile class"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid world"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Generating hex grid..."));

    const float HexWidth = TileSize * 2.f;
    const float HexHeight = TileSize * 1.732f;

    const FVector CenterLocation = FVector::ZeroVector;

    for (int32 q = -GridRadius; q <= GridRadius; ++q)
    {
        UE_LOG(LogTemp, Warning, TEXT("Processing column %d"), q);

        int32 r1 = FMath::Max(-GridRadius, -q - GridRadius);
        int32 r2 = FMath::Min(GridRadius, -q + GridRadius);

        for (int32 r = r1; r <= r2; ++r)
        {
            FVector TileLocation = ComputeTileSpawnPosition(q, r, TileSize, GridOrigin);
            FTransform SpawnTransform(TileLocation);

            AHexTile* Tile = World->SpawnActor<AHexTile>(HexTileClass, SpawnTransform);
            if (Tile)
            {
                Tile->SetCoordinates(FHexAxialCoordinates(q, r));
                Tile->SetGridManager(this);
                UE_LOG(LogTemp, Warning, TEXT("Spawned HexTile at (%d, %d)"), q, r);
            }
        }
    }
}

FVector UHexGridManager::ComputeTileSpawnPosition(int32 Q, int32 R, float LocalTileSize, const FVector& CenterLocation)
{
    constexpr float XSpacingFactor = 0.325f;  // moitié de 0.75
    constexpr float YSpacingFactor = 0.866f;

    const float HexWidth = TileSize * 2.0f;
    const float HexHeight = TileSize * YSpacingFactor;

    float XOffset = Q * HexWidth * XSpacingFactor;
    float YOffset = R * HexHeight + (Q % 2 != 0 ? HexHeight * 0.5f : 0.0f);


    float FinalX = CenterLocation.X + XOffset;
    float FinalY = CenterLocation.Y + YOffset;
    float FinalZ = CenterLocation.Z;

    FVector BasePosition(FinalX, FinalY, FinalZ);

    // Trace vertical
    FVector TraceStart = BasePosition + FVector(0.f, 0.f, 1000.f);
    FVector TraceEnd = BasePosition + FVector(0.f, 0.f, -1000.f);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.bTraceComplex = true;
    Params.AddIgnoredActor(GetOwner());

    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        FinalZ = HitResult.Location.Z + 1.f;

#if WITH_EDITOR
        DrawDebugPoint(GetWorld(), HitResult.Location, 12.0f, FColor::Yellow, false, 5.0f);
#endif

        UE_LOG(LogTemp, Warning, TEXT("Hit actor: %s"), *GetNameSafe(HitResult.GetActor()));
    }

#if WITH_EDITOR
    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 5.0f, 0, 2.0f);
#endif

    return FVector(FinalX, FinalY, FinalZ);
}
