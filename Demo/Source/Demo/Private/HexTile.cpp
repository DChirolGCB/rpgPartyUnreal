#include "HexTile.h"
#include "DemoGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"

AHexTile::AHexTile()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    TileMesh      = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
    TileMesh->SetupAttachment(RootComponent);

    // Collision impérative pour les clics
    TileMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TileMesh->SetCollisionResponseToAllChannels(ECR_Block);
    SetActorEnableCollision(true);
}

void AHexTile::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    OnClicked.AddDynamic(this, &AHexTile::HandleOnClicked);
}

void AHexTile::HandleOnClicked(AActor* TouchedActor, FKey ButtonPressed)
{
    if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->HandleTileClicked(this);
    }
}

void AHexTile::SetAxialCoordinates(const FHexAxialCoordinates& Coordinates)
{
    AxialCoordinates = Coordinates;
}

void AHexTile::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (TileMesh)
    {
        // Lève légèrement le mesh pour éviter la coplanarité avec le floor
        TileMesh->SetRelativeLocation(FVector(0.f, 0.f, VisualZOffset));
    }
}