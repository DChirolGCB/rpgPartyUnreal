#include "HexPawn.h"
#include "HexGridManager.h"
#include "HexTile.h"
#include "DemoGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Includes réels des components ICI (pas dans le .h)
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

AHexPawn::AHexPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	if (!RootComponent)
	{
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->TargetArmLength = CameraHeight;
	CameraBoom->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f)); // pitch -50°

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	TopDownCamera->SetProjectionMode(ECameraProjectionMode::Perspective);
	TopDownCamera->SetFieldOfView(60.f);
}

void AHexPawn::BeginPlay()
{
	Super::BeginPlay();

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = CameraHeight;
		CameraBoom->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f));
	}
	if (TopDownCamera)
	{
		TopDownCamera->SetProjectionMode(ECameraProjectionMode::Perspective);
	}
}

void AHexPawn::StartPathFollowing(const TArray<FHexAxialCoordinates> &InPath, UHexGridManager *InGridManager)
{
	GridRef = InGridManager;
	CurrentPath = InPath;

	int32 StartIndex = 0;
	if (CurrentTile && CurrentPath.Num() > 0)
	{
		if (CurrentPath[0] == CurrentTile->GetAxialCoordinates())
			StartIndex = 1;
	}

	if (!GridRef || !CurrentPath.IsValidIndex(StartIndex))
	{
		bIsMoving = false;
		return;
	}

	CurrentStepIndex = StartIndex;

	AHexTile *NextTile = GridRef->GetHexTileAt(CurrentPath[CurrentStepIndex]);
	if (!NextTile)
	{
		bIsMoving = false;
		return;
	}

	StartLocation = GetActorLocation();
	TargetLocation = NextTile->GetActorLocation();
	StepElapsed = 0.f;
	bIsMoving = true;
}

void AHexPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsMoving)
		return;

	StepElapsed += DeltaTime;
	float Alpha = (StepDuration > SMALL_NUMBER) ? FMath::Clamp(StepElapsed / StepDuration, 0.f, 1.f) : 1.f;
	if (bEaseInOut)
	{
		Alpha = Alpha * Alpha * (3.f - 2.f * Alpha);
	} // smoothstep

	const FVector NewLoc = FMath::Lerp(StartLocation, TargetLocation, Alpha);
	SetActorLocation(NewLoc);

	// Pas de rotation (bFaceDirection=false par défaut) — on laisse le bloc conditionnel
	if (bFaceDirection)
	{
		const FVector Dir2D(TargetLocation.X - NewLoc.X, TargetLocation.Y - NewLoc.Y, 0.f);
		if (!Dir2D.IsNearlyZero())
		{
			const FRotator Cur = GetActorRotation();
			const FRotator Want = Dir2D.Rotation();
			const float DeltaYaw = FMath::FindDeltaAngleDegrees(Cur.Yaw, Want.Yaw);
			const float MaxStep = TurnRateDegPerSec * DeltaTime;
			SetActorRotation(FRotator(0.f, Cur.Yaw + FMath::Clamp(DeltaYaw, -MaxStep, MaxStep), 0.f));
		}
	}

	if (Alpha >= 1.f - KINDA_SMALL_NUMBER)
	{
		SetActorLocation(TargetLocation); // snap

		if (GridRef && CurrentPath.IsValidIndex(CurrentStepIndex))
		{
			if (AHexTile *Landed = GridRef->GetHexTileAt(CurrentPath[CurrentStepIndex]))
				CurrentTile = Landed;
		}

		++CurrentStepIndex;

		if (!GridRef || !CurrentPath.IsValidIndex(CurrentStepIndex))
		{
			bIsMoving = false;
			return;
		}

		AHexTile *NextTile = GridRef->GetHexTileAt(CurrentPath[CurrentStepIndex]);
		if (CurrentTile && GridRef && NextTile)
		{
			const FHexAxialCoordinates Cur = CurrentTile->GetAxialCoordinates();
			const FHexAxialCoordinates Next = CurrentPath[CurrentStepIndex];

			// Adjacence stricte axiale (et tuile existante)
			const TArray<FHexAxialCoordinates> Neigh = GridRef->GetNeighbors(Cur);
			const bool bAdjacent = Neigh.Contains(Next);
			if (!bAdjacent || !GridRef->GetHexTileAt(Next))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Move] Step non-voisin ou absent: (%d,%d)->(%d,%d). Stop."),
					   Cur.Q, Cur.R, Next.Q, Next.R);
				bIsMoving = false;
				return;
			}
		}

		if (!NextTile)
		{
			bIsMoving = false;
			return;
		}

		StartLocation = GetActorLocation();
		TargetLocation = NextTile->GetActorLocation();
		StepElapsed = 0.f;
		bIsMoving = true;
	}
}

void AHexPawn::SetCurrentTile(AHexTile *NewTile)
{
	CurrentTile = NewTile;
}

void AHexPawn::InitializePawnStartTile(const FHexAxialCoordinates &StartCoords)
{
	UWorld *World = GetWorld();
	if (!World)
		return;

	ADemoGameMode *GameMode = World->GetAuthGameMode<ADemoGameMode>();
	if (!GameMode)
		return;

	UHexGridManager *GridManager = GameMode->GetHexGridManager();
	if (!GridManager)
		return;

	AHexTile *Tile = GridManager->GetHexTileAt(StartCoords);
	if (!Tile)
	{
		UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile: No tile at (%d, %d)"), StartCoords.Q, StartCoords.R);
		return;
	}

	CurrentTile = Tile;
	SetActorLocation(Tile->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("Pawn initialized on tile (%d, %d)"), StartCoords.Q, StartCoords.R);
}
