#include "HexPawn.h"
#include "HexGridManager.h"
#include "HexTile.h"
#include "DemoGameMode.h"
#include "HexAnimationTypes.h"
#include "Kismet/GameplayStatics.h"
#include "HexSpriteComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

// Includes réels des components ICI (pas dans le .h)
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

AHexPawn::AHexPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
    NetUpdateFrequency = 60.f;
    MinNetUpdateFrequency = 30.f;
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

	SpriteComp = CreateDefaultSubobject<UHexSpriteComponent>(TEXT("SpriteComp"));
	SpriteComp->SetupAttachment(RootComponent);
}

void AHexPawn::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->bAutoManageActiveCameraTarget = false;
		PC->SetViewTarget(this); // immédiat
	}
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = CameraHeight;
		CameraBoom->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
	}
	if (TopDownCamera)
	{
		TopDownCamera->SetProjectionMode(ECameraProjectionMode::Perspective);
	}
	if (!SpriteComp || SpriteComp->HasAnyFlags(RF_ClassDefaultObject) || SpriteComp->GetOwner() != this)
    {
        if (UHexSpriteComponent* Found = FindComponentByClass<UHexSpriteComponent>())
        {
            SpriteComp = Found;
            UE_LOG(LogTemp, Warning, TEXT("[Sprite] Rebound to instance comp: %s (owner=%s)"),
                   *GetNameSafe(SpriteComp), *GetNameSafe(SpriteComp->GetOwner()));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[Sprite] No UHexSpriteComponent found on pawn instance"));
        }
    }

    if (SpriteComp)
    {
        SpriteComp->SetPlayRate(1.f);
        SpriteComp->SetAnimationState(EHexAnimState::Idle);
        UE_LOG(LogTemp, Warning, TEXT("[Sprite] Idle=%s Walk=%s Flipbook=%s Owner=%s IsCDO=%d"),
            *GetNameSafe(SpriteComp->IdleAnim),
            *GetNameSafe(SpriteComp->WalkAnim),
            *GetNameSafe(SpriteComp->GetFlipbook()),
            *GetNameSafe(SpriteComp->GetOwner()),
            SpriteComp->HasAnyFlags(RF_ClassDefaultObject) ? 1 : 0);
    }

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    { PC->bAutoManageActiveCameraTarget = false; PC->SetViewTarget(this); }
}

void AHexPawn::StartPathFollowing(const TArray<FHexAxialCoordinates> &InPath, UHexGridManager *InGridManager)
{
	GridRef = InGridManager;
	CurrentPath = InPath;
	bIsMoving = true;
	if (SpriteComp && HasAuthority())
	{
		SpriteComp->SetAnimationState(EHexAnimState::Walking);
		UE_LOG(LogTemp, Warning, TEXT("[Anim] -> Walking (StartPathFollowing)"));
	}
	int32 StartIndex = 0;
	if (CurrentTile && CurrentPath.Num() > 0)
	{
		if (CurrentPath[0] == CurrentTile->GetAxialCoordinates())
			StartIndex = 1;
	}

	if (!GridRef || !CurrentPath.IsValidIndex(StartIndex))
	{
		bIsMoving = false;
		if (SpriteComp && HasAuthority())
		{
			SpriteComp->SetAnimationState(EHexAnimState::Idle);
			UE_LOG(LogTemp, Warning, TEXT("[Anim] -> Idle (stop)"));
		}
		return;
	}

	CurrentStepIndex = StartIndex;

	AHexTile *NextTile = GridRef->GetHexTileAt(CurrentPath[CurrentStepIndex]);
	if (!NextTile)
	{
		bIsMoving = false;
		if (SpriteComp && HasAuthority())
		{
			SpriteComp->SetAnimationState(EHexAnimState::Idle);
			UE_LOG(LogTemp, Warning, TEXT("[Anim] -> Idle (stop)"));
		}
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
	{
		if (SpriteComp)
			SpriteComp->SetAnimationState(EHexAnimState::Idle);
		return;
	}

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
			if (SpriteComp && HasAuthority())
			{
				SpriteComp->SetAnimationState(EHexAnimState::Idle);
				UE_LOG(LogTemp, Warning, TEXT("[Anim] -> Idle (stop)"));
			}
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
			// Debug: liste les voisins et la distance
			{
				FString NeighStr;
				for (const auto &N : Neigh)
				{
					NeighStr += FString::Printf(TEXT(" (%d,%d)"), N.Q, N.R);
				}
				const int32 Dist = Cur.DistanceTo(Next);
				UE_LOG(LogTemp, Verbose, TEXT("[Move] Check step %d: (%d,%d)->(%d,%d) | Dist=%d | Neigh:%s"),
					   CurrentStepIndex, Cur.Q, Cur.R, Next.Q, Next.R, Dist, *NeighStr);
			}
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
			if (SpriteComp && HasAuthority())
			{
				SpriteComp->SetAnimationState(EHexAnimState::Idle);
				UE_LOG(LogTemp, Warning, TEXT("[Anim] -> Idle (stop)"));
			}
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

void AHexPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHexPawn, ReplicatedPath);
}

void AHexPawn::OnRep_CurrentPath()
{
	// Quand le path répliqué arrive sur client → lancer déplacement
	if (GridRef && ReplicatedPath.Num() > 0)
	{
		StartPathFollowing(ReplicatedPath, GridRef);
	}
}

void AHexPawn::ServerRequestMove_Implementation(const TArray<FHexAxialCoordinates> &NewPath)
{
	ReplicatedPath = NewPath;
	StartPathFollowing(NewPath, GridRef);
}

void AHexPawn::PossessedBy(AController *NewController)
{
	Super::PossessedBy(NewController);
	if (APlayerController *PC = Cast<APlayerController>(NewController))
	{
		PC->bAutoManageActiveCameraTarget = false;
		PC->SetViewTarget(this); // serveur
	}
}

void AHexPawn::OnRep_Controller()
{
	Super::OnRep_Controller();
	if (APlayerController *PC = Cast<APlayerController>(GetController()))
	{
		PC->bAutoManageActiveCameraTarget = false;
		PC->ClientSetViewTarget(this); // client
	}
}
