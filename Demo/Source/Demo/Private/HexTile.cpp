#include "HexTile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DemoGameMode.h"

AHexTile::AHexTile()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;

    TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
    TileMesh->SetupAttachment(SceneRoot);

    // Pas de label, pas de rotation, juste une tuile cliquable
    SetActorTickEnabled(false);
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
