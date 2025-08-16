// HexPawn.cpp
#include "HexPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "HexAnimationTypes.h"
#include "HexGridManager.h"
#include "HexSpriteComponent.h"
#include "HexTile.h"
#include "DemoGameMode.h"

AHexPawn::AHexPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetNetUpdateFrequency(60.f);
    SetMinNetUpdateFrequency(30.f);

    if (!RootComponent)
    {
        RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->TargetArmLength = CameraHeight;
    CameraBoom->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f));

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

    // Rebind to instance sprite if the CDO component leaked through
    if (!SpriteComp || SpriteComp->HasAnyFlags(RF_ClassDefaultObject) || SpriteComp->GetOwner() != this)
    {
        if (UHexSpriteComponent *Found = FindComponentByClass<UHexSpriteComponent>())
        {
            SpriteComp = Found;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[Sprite] Missing UHexSpriteComponent on instance"));
        }
    }

    if (SpriteComp)
    {
        SpriteBaseScale = SpriteComp->GetRelativeScale3D().GetAbs();
        SpriteComp->SetPlayRate(1.f);
        SpriteComp->SetAnimationState(EHexAnimState::Idle);
    }

    if (CameraBoom)
    {
        CameraBoom->TargetArmLength = CameraHeight;
        CameraBoom->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
    }

    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bAutoManageActiveCameraTarget = false;
        PC->SetViewTarget(this);
    }
}

void AHexPawn::UpdateSpriteMirrorToward(const FVector &From, const FVector &To)
{
    if (!SpriteComp)
        return;

    const FVector2D MoveDir2D = FVector2D(To - From).GetSafeNormal();
    if (MoveDir2D.IsNearlyZero())
        return;

    const FVector2D CamRight2D = FVector2D(TopDownCamera ? TopDownCamera->GetRightVector() : FVector::RightVector);
    const bool bGoingRight = FVector2D::DotProduct(MoveDir2D, CamRight2D) >= 0.f;

    FVector S = SpriteBaseScale;
    S.X = bGoingRight ? FMath::Abs(S.X) : -FMath::Abs(S.X); // horizontal flip
    SpriteComp->SetRelativeScale3D(S);
}

void AHexPawn::StartPathFollowing(const TArray<FHexAxialCoordinates> &InPath, UHexGridManager *InGridManager)
{
    GridRef = InGridManager;
    CurrentPath = InPath;

    int32 StartIndex = 0;
    if (CurrentTile && CurrentPath.Num() > 0 && CurrentPath[0] == CurrentTile->GetAxialCoordinates())
    {
        StartIndex = 1;
    }

    if (!GridRef || !CurrentPath.IsValidIndex(StartIndex))
    {
        bIsMoving = false;
        if (HasAuthority() && SpriteComp)
            SpriteComp->SetAnimationState(EHexAnimState::Idle);
        return;
    }

    CurrentStepIndex = StartIndex;

    AHexTile *NextTile = GridRef->GetHexTileAt(CurrentPath[CurrentStepIndex]);
    if (!NextTile)
    {
        bIsMoving = false;
        if (HasAuthority() && SpriteComp)
            SpriteComp->SetAnimationState(EHexAnimState::Idle);
        return;
    }

    StartLocation = GetActorLocation();
    TargetLocation = NextTile->GetActorLocation();
    StepElapsed = 0.f;
    bIsMoving = true;

    UpdateSpriteMirrorToward(StartLocation, TargetLocation);
    if (HasAuthority() && SpriteComp)
        SpriteComp->SetAnimationState(EHexAnimState::Walking);
}

void AHexPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsMoving)
    {
        if (HasAuthority() && SpriteComp)
            SpriteComp->SetAnimationState(EHexAnimState::Idle);
        return;
    }

    StepElapsed += DeltaTime;
    float Alpha = (StepDuration > SMALL_NUMBER) ? FMath::Clamp(StepElapsed / StepDuration, 0.f, 1.f) : 1.f;
    if (bEaseInOut)
    {
        Alpha = Alpha * Alpha * (3.f - 2.f * Alpha); // smoothstep
    }

    const FVector NewLoc = FMath::Lerp(StartLocation, TargetLocation, Alpha);
    SetActorLocation(NewLoc);

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
        SetActorLocation(TargetLocation);

        if (GridRef && CurrentPath.IsValidIndex(CurrentStepIndex))
        {
            if (AHexTile *Landed = GridRef->GetHexTileAt(CurrentPath[CurrentStepIndex]))
            {
                CurrentTile = Landed;
            }
        }

        ++CurrentStepIndex;

        if (!GridRef || !CurrentPath.IsValidIndex(CurrentStepIndex))
        {
            bIsMoving = false;
            if (HasAuthority() && SpriteComp)
                SpriteComp->SetAnimationState(EHexAnimState::Idle);
            return;
        }

        AHexTile *NextTile = GridRef->GetHexTileAt(CurrentPath[CurrentStepIndex]);
        if (!NextTile)
        {
            bIsMoving = false;
            if (HasAuthority() && SpriteComp)
                SpriteComp->SetAnimationState(EHexAnimState::Idle);
            return;
        }

        // Optional sanity: require adjacency
        if (CurrentTile && GridRef)
        {
            const FHexAxialCoordinates Cur = CurrentTile->GetAxialCoordinates();
            const FHexAxialCoordinates Next = CurrentPath[CurrentStepIndex];
            const bool bAdjacent = GridRef->GetNeighbors(Cur).Contains(Next);
            if (!bAdjacent || !GridRef->GetHexTileAt(Next))
            {
                UE_LOG(LogTemp, Warning, TEXT("[Move] Invalid step: (%d,%d)->(%d,%d). Stop."),
                       Cur.Q, Cur.R, Next.Q, Next.R);
                bIsMoving = false;
                if (HasAuthority() && SpriteComp)
                    SpriteComp->SetAnimationState(EHexAnimState::Idle);
                return;
            }
        }

        StartLocation = GetActorLocation();
        TargetLocation = NextTile->GetActorLocation();
        StepElapsed = 0.f;
        bIsMoving = true;

        UpdateSpriteMirrorToward(StartLocation, TargetLocation);
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

    ADemoGameMode *GM = World->GetAuthGameMode<ADemoGameMode>();
    if (!GM)
        return;

    UHexGridManager *Grid = GM->GetHexGridManager();
    if (!Grid)
        return;

    AHexTile *Tile = Grid->GetHexTileAt(StartCoords);
    if (!Tile)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile: No tile at (%d,%d)"), StartCoords.Q, StartCoords.R);
        return;
    }

    CurrentTile = Tile;
    SetActorLocation(Tile->GetActorLocation());
    UE_LOG(LogTemp, Warning, TEXT("Pawn initialized on tile (%d,%d)"), StartCoords.Q, StartCoords.R);
}

void AHexPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHexPawn, ReplicatedPath);
}

void AHexPawn::OnRep_CurrentPath()
{
    if (!GridRef)
    {
        if (ADemoGameMode *GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADemoGameMode>() : nullptr)
        {
            GridRef = GM ? GM->GetHexGridManager() : nullptr;
        }
    }
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
        PC->SetViewTarget(this);
    }
}

void AHexPawn::OnRep_Controller()
{
    Super::OnRep_Controller();
    if (APlayerController *PC = Cast<APlayerController>(GetController()))
    {
        PC->bAutoManageActiveCameraTarget = false;
        PC->ClientSetViewTarget(this);
    }
}
