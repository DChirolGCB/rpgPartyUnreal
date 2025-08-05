#include "HexPawn.h"
#include "HexGridManager.h"
#include "HexTile.h"
#include "Components/TimelineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DemoGameMode.h"

AHexPawn::AHexPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    MovementTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MovementTimeline"));
}

void AHexPawn::BeginPlay()
{
    Super::BeginPlay();

    if (MovementCurve)
    {
        UpdateFunction.BindUFunction(this, FName("OnMovementUpdate"));
        FinishedFunction.BindUFunction(this, FName("OnMovementComplete"));

        MovementTimeline->AddInterpFloat(MovementCurve, UpdateFunction);
        MovementTimeline->SetTimelineFinishedFunc(FinishedFunction);
        MovementTimeline->SetLooping(false);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MovementCurve not set on %s"), *GetName());
    }
}

void AHexPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (MovementTimeline)
    {
        MovementTimeline->TickComponent(DeltaTime, ELevelTick::LEVELTICK_TimeOnly, nullptr);
    }
}

void AHexPawn::StartPathFollowing(const TArray<FHexAxialCoordinates>& InPath)
{
    if (InPath.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Empty path"));
        return;
    }

    PathToFollow = InPath;
    CurrentStepIndex = 0;

    MoveToNextStep();
}

void AHexPawn::MoveToNextStep()
{
    if (CurrentStepIndex >= PathToFollow.Num())
    {
        OnMovementComplete();
        return;
    }

    // Récupération de la tuile cible via coordonnées
    const FHexAxialCoordinates& TargetCoords = PathToFollow[CurrentStepIndex];

    UWorld* World = GetWorld();
	if (!World) return;

	ADemoGameMode* GameMode = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(World));
	if (!GameMode) return;

	UHexGridManager* GridManager = GameMode->GetHexGridManager();
	if (!GridManager) return;

    AActor* TileActor = GridManager->GetHexTileAt(TargetCoords);
	AHexTile* TargetTile = Cast<AHexTile>(TileActor);
	if (!TargetTile) return;

    StartLocation = GetActorLocation();
    TargetLocation = TargetTile->GetActorLocation();

    CurrentStepIndex++;

    MovementTimeline->PlayFromStart();
}

void AHexPawn::OnMovementUpdate(float Alpha)
{
    FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, Alpha);
    SetActorLocation(NewLocation);
}

void AHexPawn::OnMovementComplete()
{
    SetActorLocation(TargetLocation);

    // MAJ de la tuile courante
    UWorld* World = GetWorld();
    if (!World) return;

    UHexGridManager* GridManager = Cast<UHexGridManager>(UGameplayStatics::GetActorOfClass(World, UHexGridManager::StaticClass()));
    if (!GridManager) return;

    FHitResult Hit;
    FVector TraceStart = GetActorLocation() + FVector(0, 0, 100);
    FVector TraceEnd = GetActorLocation() + FVector(0, 0, -1000);

    if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility))
    {
        AHexTile* HitTile = Cast<AHexTile>(Hit.GetActor());
        if (HitTile)
        {
            CurrentTile = HitTile;
        }
    }

    // Si on a encore des étapes
    if (CurrentStepIndex < PathToFollow.Num())
    {
        MoveToNextStep();
    }
}
