// Fill out your copyright notice in the Description page of Project Settings.


#include "HexTile.h"

// Sets default values
AHexTile::AHexTile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	TileMesh->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AHexTile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHexTile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHexTile::SetGridManager(UHexGridManager* InManager)
{
    GridManager = InManager;
}

void AHexTile::SetCoordinates(FHexAxialCoordinates InCoords)
{
    AxialCoords = InCoords;
}

FHexAxialCoordinates AHexTile::GetAxialCoordinates() const
{
    return AxialCoords;
}