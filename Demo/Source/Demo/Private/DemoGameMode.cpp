// DemoGameMode.cpp
#include "DemoGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Algo/Reverse.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

#include "BattleWidget.h"
#include "HexAnimationManager.h"
#include "HexGridManager.h"
#include "HexPathFinder.h"
#include "HexPawn.h"
#include "HexTile.h"
#include "PathView.h"
#include "PlayerStatsWidget.h"
#include "LoadoutEditorWidget.h"
#include "HexEnemyPawn.h" // <-- required
#include "EnemyDefinition.h"
#include "BattleWidget.h"
#include "Engine/Texture2D.h"

namespace
{
    constexpr float kSnapRetryPeriodSec = 0.05f;
    constexpr float kPreviewThrottleSec = 0.10f;
}

namespace HexBridge
{
    /** BFS across existing tiles to bridge non-adjacent steps */
    static bool BuildBridge(UHexGridManager *GM,
                            const FHexAxialCoordinates &From,
                            const FHexAxialCoordinates &To,
                            TArray<FHexAxialCoordinates> &OutChain)
    {
        if (!GM)
            return false;
        if (From == To)
        {
            OutChain.Reset();
            return true;
        }

        TQueue<FHexAxialCoordinates> Open;
        TMap<FHexAxialCoordinates, FHexAxialCoordinates> Parent;

        Open.Enqueue(From);
        Parent.Add(From, From);

        bool bFound = false;
        FHexAxialCoordinates Cur;

        while (Open.Dequeue(Cur))
        {
            const TArray<FHexAxialCoordinates> Neigh = GM->GetNeighbors(Cur);
            for (const FHexAxialCoordinates &N : Neigh)
            {
                if (Parent.Contains(N))
                    continue;
                Parent.Add(N, Cur);
                if (N == To)
                {
                    bFound = true;
                    Cur = N;
                    break;
                }
                Open.Enqueue(N);
            }
            if (bFound)
                break;
        }

        if (!bFound)
            return false;

        TArray<FHexAxialCoordinates> Chain;
        for (FHexAxialCoordinates C = Cur; !(C == From); C = Parent[C])
        {
            Chain.Add(C);
        }
        Algo::Reverse(Chain);
        OutChain = MoveTemp(Chain);
        return true;
    }

    /** Enforce adjacency by inserting intermediate tiles using BFS on existing tiles */
    static void BridgePathUsingExistingTiles(UHexGridManager *GM, TArray<FHexAxialCoordinates> &Path)
    {
        if (!GM || Path.Num() < 2)
            return;

        TArray<FHexAxialCoordinates> Out;
        Out.Reserve(Path.Num() * 2);
        Out.Add(Path[0]);

        for (int32 i = 1; i < Path.Num(); ++i)
        {
            const FHexAxialCoordinates From = Out.Last();
            const FHexAxialCoordinates To = Path[i];

            const bool bAdjacent = GM->GetNeighbors(From).Contains(To);
            if (bAdjacent)
            {
                Out.Add(To);
                continue;
            }

            TArray<FHexAxialCoordinates> Bridge;
            if (BuildBridge(GM, From, To, Bridge) && Bridge.Num() > 0)
            {
                Out.Append(Bridge);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Bridge] Impossible de relier (%d,%d)->(%d,%d) via voisins — saut ignoré"),
                       From.Q, From.R, To.Q, To.R);
            }
        }

        Path = MoveTemp(Out);
    }
}

namespace Hex
{
    /** Doubled-Q neighbor deltas */
    static const FHexAxialCoordinates NeighborDirs[6] =
        {
            {+2, 0}, {+1, -1}, {-1, -1}, {-2, 0}, {-1, +1}, {+1, +1}};

    static int32 HexDistance(const FHexAxialCoordinates &A, const FHexAxialCoordinates &B)
    {
        const int32 dq = FMath::Abs(A.Q - B.Q);
        const int32 dr = FMath::Abs(A.R - B.R);
        const int32 diag = FMath::Min(dq / 2, dr);
        const int32 remQ = dq - diag * 2;
        const int32 remR = dr - diag;
        return diag + remR + remQ / 2;
    }

    static bool AreNeighbors(const FHexAxialCoordinates &A, const FHexAxialCoordinates &B)
    {
        for (const auto &D : NeighborDirs)
        {
            if (A.Q + D.Q == B.Q && A.R + D.R == B.R)
                return true;
        }
        return false;
    }

    static FHexAxialCoordinates StepToward(const FHexAxialCoordinates &From, const FHexAxialCoordinates &To)
    {
        int32 BestIdx = 0;
        int32 BestDist = TNumericLimits<int32>::Max();
        for (int32 i = 0; i < 6; ++i)
        {
            FHexAxialCoordinates C{From.Q + NeighborDirs[i].Q, From.R + NeighborDirs[i].R};
            const int32 Dist = HexDistance(C, To);
            if (Dist < BestDist)
            {
                BestDist = Dist;
                BestIdx = i;
            }
        }
        return {From.Q + NeighborDirs[BestIdx].Q, From.R + NeighborDirs[BestIdx].R};
    }

    /** Expand a path by inserting steps until all hops are adjacent (guarded) */
    static void SanitizePath(TArray<FHexAxialCoordinates> &Path)
    {
        if (Path.Num() < 2)
            return;

        TArray<FHexAxialCoordinates> Out;
        Out.Reserve(Path.Num() * 2);
        Out.Add(Path[0]);

        for (int32 i = 1; i < Path.Num(); ++i)
        {
            FHexAxialCoordinates Cur = Out.Last();
            const FHexAxialCoordinates Target = Path[i];

            int32 Guard = 0;
            while (!AreNeighbors(Cur, Target) && Guard++ < 64)
            {
                const FHexAxialCoordinates Step = StepToward(Cur, Target);
                Out.Add(Step);
                Cur = Step;
            }
            Out.Add(Target);

#if !UE_BUILD_SHIPPING
            const int32 Jump = HexDistance(Out[Out.Num() - 2], Out.Last());
            if (Jump > 1)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SanitizePath] Jump=%d entre (%d,%d)->(%d,%d)"),
                       Jump, Out[Out.Num() - 2].Q, Out[Out.Num() - 2].R, Out.Last().Q, Out.Last().R);
            }
#endif
        }

        Path = MoveTemp(Out);
    }
}

void ADemoGameMode::PostLogin(APlayerController *NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // Retry until snap succeeds
    GetWorldTimerManager().SetTimer(
        SnapRetryHandle, this, &ADemoGameMode::TrySnapPawnOnce,
        kSnapRetryPeriodSec, /*bLoop=*/true, /*FirstDelay=*/0.0f);
}

void ADemoGameMode::Logout(AController *Exiting)
{
    if (AHexPawn *Pawn = Cast<AHexPawn>(Exiting->GetPawn()))
    {
        if (AnimationManager)
        {
            AnimationManager->UnregisterPlayer(Pawn);
        }
    }
    Super::Logout(Exiting);
}

ADemoGameMode::ADemoGameMode()
{
    DefaultPawnClass = AHexPawn::StaticClass();

    GridManager = CreateDefaultSubobject<UHexGridManager>(TEXT("HexGridManager"));
    PathFinder = CreateDefaultSubobject<UHexPathFinder>(TEXT("HexPathFinder"));

    static ConstructorHelpers::FClassFinder<AHexTile> TileBP(TEXT("/Game/Blueprints/BP_HexTile"));
    if (TileBP.Succeeded())
    {
        HexTileClass = TileBP.Class;
    }

    static ConstructorHelpers::FObjectFinder<UEnemyDefinition> Gob(
        TEXT("/Game/Datasets/DA_Goblux.DA_Goblux")); // adjust path if different
    if (Gob.Succeeded())
    {
        EnemyCatalog.Add(TEXT("goblux"), Gob.Object);
    }
}

void ADemoGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (APlayerController *PC = GetWorld()->GetFirstPlayerController())
    {
        PC->InputComponent->BindAction("TestBattle", IE_Pressed, this, &ADemoGameMode::StartTestBattle);
    }
    if (!GridManager || !GridManager->IsRegistered())
    {
        GridManager = NewObject<UHexGridManager>(this, TEXT("HexGridManager_RT"));
        AddInstanceComponent(GridManager);
        GridManager->RegisterComponent();
    }
    if (!PathFinder || !PathFinder->IsRegistered())
    {
        PathFinder = NewObject<UHexPathFinder>(this, TEXT("HexPathFinder_RT"));
        AddInstanceComponent(PathFinder);
        PathFinder->RegisterComponent();
    }
    if (PlayerStatsWidgetClass)
    {
        if (UUserWidget *W = CreateWidget<UUserWidget>(GetWorld(), PlayerStatsWidgetClass))
        {
            W->AddToViewport(/*ZOrder*/ 0);
            PlayerStatsWidget = W;

            // wire data source
            if (AHexPawn *P = GetPlayerPawnTyped())
            {
                // If widget is our C++ type, push the Combat component
                if (UPlayerStatsWidget *PSW = Cast<UPlayerStatsWidget>(W))
                {
                    PSW->SetCombat(P->GetCombat());
                }
            }
        }
    }
    EnemyRNG.Initialize(int32(FDateTime::UtcNow().GetTicks() & 0xFFFFFFFF));

    UWorld *W = GetWorld();
    if (!W)
        return;

    const bool bIsGame = (W->IsGameWorld() || W->WorldType == EWorldType::PIE || W->WorldType == EWorldType::Game);
    if (!bIsGame || IsRunningCommandlet() || HasAnyFlags(RF_ClassDefaultObject))
    {
        return;
    }

    if (PathFinder && GridManager)
    {
        PathFinder->Init(GridManager);
        UE_LOG(LogTemp, Warning, TEXT("[PathFinder] Bound Grid=%s"), *GetNameSafe(GridManager));
    }

    // Optional PC override coming from BP
    if (!PCClassSoft.IsNull())
    {
        if (UClass *PCCls = PCClassSoft.LoadSynchronous())
        {
            PlayerControllerClass = PCCls;
        }
    }

    if (APlayerController *PC0 = GetWorld()->GetFirstPlayerController())
    {
        UE_LOG(LogTemp, Warning, TEXT("PC actif: %s"), *PC0->GetClass()->GetName());
    }
    UE_LOG(LogTemp, Warning, TEXT("GM actif: %s"), *GetClass()->GetName());

    APlayerController *OldPC = GetWorld()->GetFirstPlayerController();
    if (OldPC)
    {
        UE_LOG(LogTemp, Warning, TEXT("PC actif: %s (attendu: %s)"),
               *OldPC->GetClass()->GetName(),
               PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("(none)"));
    }

    // Replace PC if level did not take the right class
    if (PlayerControllerClass && OldPC && !OldPC->IsA(PlayerControllerClass))
    {
        APawn *Pawn = OldPC->GetPawn();

        FActorSpawnParameters Params;
        Params.Instigator = Pawn;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        APlayerController *NewPC = GetWorld()->SpawnActor<APlayerController>(
            PlayerControllerClass,
            Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector,
            Pawn ? Pawn->GetActorRotation() : FRotator::ZeroRotator,
            Params);

        if (NewPC)
        {
            RestartPlayer(NewPC);
            if (Pawn)
            {
                OldPC->UnPossess();
                NewPC->Possess(Pawn);
            }
            UE_LOG(LogTemp, Warning, TEXT("PC remplacé: %s -> %s"),
                   *OldPC->GetClass()->GetName(), *NewPC->GetClass()->GetName());
            OldPC->Destroy();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Echec spawn PlayerControllerClass %s"), *PlayerControllerClass->GetName());
        }
    }

    if (!ensure(GridManager))
    {
        UE_LOG(LogTemp, Error, TEXT("DemoGameMode BeginPlay: GridManager is NULL, aborting grid gen"));
        return;
    }
    if (!ensure(HexTileClass))
    {
        UE_LOG(LogTemp, Error, TEXT("DemoGameMode BeginPlay: HexTileClass is NULL, aborting grid gen"));
        return;
    }

    // Build grid
    GridManager->InitializeGrid(GridRadius, HexTileClass);
    PathFinder->Init(GridManager);

    UE_LOG(LogTemp, Warning, TEXT("DemoGameMode BeginPlay : grille générée"));

    // Initial pawn tile
    InitializePawnStartTile(FHexAxialCoordinates(0, 6));

    // Mouse and UI setup
    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }

    if (!IsValid(PathView))
    {
        PathView = GetWorld()->SpawnActor<APathView>();
    }

    UE_LOG(LogTemp, Warning, TEXT("PC=%s  Pawn=%s  ViewTarget=%s"),
           *GetNameSafe(GetWorld()->GetFirstPlayerController()),
           *GetNameSafe(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr),
           *GetNameSafe(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetViewTarget() : nullptr));
}

void ADemoGameMode::HandleTileClicked(AHexTile *ClickedTile)
{
    if (!ClickedTile || !GridManager || !PathFinder)
        return;

    // Shop tiles open UI and skip pathfinding
    if (ClickedTile->GetTileType() == EHexTileType::Shop || ClickedTile->bIsShop || ClickedTile->ActorHasTag(TEXT("Shop")))
    {
        OpenShopAt(ClickedTile);
        return;
    }

    AHexPawn *HexP = GetPlayerPawnTyped();
    if (!HexP)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleTileClicked: Pawn n'est pas un AHexPawn"));
        return;
    }

    AHexTile *FromTile = HexP->GetCurrentTile();
    if (!FromTile)
{
    HexP->SetCurrentTile(ClickedTile);
    HexP->SetActorLocation(ClickedTile->GetActorLocation());
    UE_LOG(LogTemp, Warning, TEXT("Affectation initiale -> (%d,%d)"),
           ClickedTile->GetAxialCoordinates().Q, ClickedTile->GetAxialCoordinates().R);

    if (ClickedTile->GetTileType() == EHexTileType::Enemy)
    {
        StartTestBattle();
        // keep tile as Enemy as requested
        // ClickedTile->SetTileType(EHexTileType::Normal); // do NOT reset now
    }
    UpdateReachableVisibility(3);
    return;
}


    const FHexAxialCoordinates Start = FromTile->GetAxialCoordinates();
    const FHexAxialCoordinates Goal = ClickedTile->GetAxialCoordinates();
    if (Start == Goal)
        return;

    UE_LOG(LogTemp, Warning, TEXT("A*: Start=(%d,%d) Goal=(%d,%d)"),
           Start.Q, Start.R, Goal.Q, Goal.R);
    UE_LOG(LogTemp, Warning, TEXT("A*: StartExists=%d GoalExists=%d StartNeigh=%d GoalNeigh=%d"),
           GridManager->GetHexTileAt(Start) != nullptr,
           GridManager->GetHexTileAt(Goal) != nullptr,
           GridManager->GetNeighbors(Start).Num(),
           GridManager->GetNeighbors(Goal).Num());

    TArray<FHexAxialCoordinates> Path = PathFinder->FindPath(Start, Goal);
    HexBridge::BridgePathUsingExistingTiles(GridManager, Path);

    if (Path.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("A*: aucun chemin"));
        return;
    }

    HexP->StartPathFollowing(Path, GridManager);
}

void ADemoGameMode::InitializePawnStartTile(const FHexAxialCoordinates &InStartCoords)
{
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : GridManager null"));
        return;
    }

    AHexTile *Tile = GridManager->GetHexTileAt(InStartCoords);
    if (!Tile)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : pas de tuile à (%d,%d)"),
               InStartCoords.Q, InStartCoords.R);
        return;
    }

    if (AHexPawn *HexP = GetPlayerPawnTyped())
    {
        HexP->SetCurrentTile(Tile);
        UpdateReachableVisibility(3);
        UE_LOG(LogTemp, Warning, TEXT("Pawn démarré sur (%d,%d)"),
               InStartCoords.Q, InStartCoords.R);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InitializePawnStartTile : pawn non HexPawn"));
    }
}

void ADemoGameMode::ShowPlannedPathTo(AHexTile *GoalTile)
{
    if (!GridManager || !PathFinder || !GoalTile || !PathView)
        return;

    AHexPawn *P = GetPlayerPawnTyped();
    AHexTile *StartTile = P ? P->GetCurrentTile() : nullptr;
    if (!StartTile)
    {
        PathView->Clear();
        return;
    }

    const FHexAxialCoordinates Start = StartTile->GetAxialCoordinates();
    const FHexAxialCoordinates Goal = GoalTile->GetAxialCoordinates();

    TArray<FHexAxialCoordinates> AxialPath = PathFinder->FindPath(Start, Goal);
    if (AxialPath.Num() < 2)
    {
        PathView->Clear();
        return;
    }

    TArray<FVector> Points;
    Points.Reserve(AxialPath.Num());
    for (const auto &C : AxialPath)
        if (AHexTile *T = GridManager->GetHexTileAt(C))
            Points.Add(T->GetActorLocation());

    PathView->Show(Points);
}

void ADemoGameMode::ClearPlannedPath()
{
    if (PathView)
        PathView->Clear();
}

AHexPawn *ADemoGameMode::GetPlayerPawnTyped() const
{
    return Cast<AHexPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void ADemoGameMode::PreviewPathTo(AHexTile *GoalTile)
{
    if (!bPreviewEnabled || !GoalTile)
        return;
    PendingGoal = GoalTile;

    if (!GetWorldTimerManager().IsTimerActive(PreviewThrottle))
    {
        GetWorldTimerManager().SetTimer(PreviewThrottle, this, &ADemoGameMode::DoPreviewTick, kPreviewThrottleSec, false);
    }
}

void ADemoGameMode::DoPreviewTick()
{
    if (!PathView)
        return;

    if (!bPreviewEnabled)
    {
        ClearPreview();
        return;
    }

    UHexGridManager *GM = GetHexGridManager();
    UHexPathFinder *PF = GetHexPathFinder();
    if (!GM || !PF)
        return;

    AHexPawn *P = GetPlayerPawnTyped();
    if (!P)
        return;

    AHexTile *StartTile = P->GetCurrentTile();
    AHexTile *GoalTile = PendingGoal.IsValid() ? PendingGoal.Get() : nullptr;
    if (!StartTile || !GoalTile)
        return;

    const FHexAxialCoordinates Start = StartTile->GetAxialCoordinates();
    const FHexAxialCoordinates Goal = GoalTile->GetAxialCoordinates();

    if (Start == LastStart && Goal == LastGoal)
        return;
    LastStart = Start;
    LastGoal = Goal;

    TArray<FHexAxialCoordinates> AxialPath = PF->FindPath(Start, Goal);
    if (AxialPath.Num() < 2)
    {
        ClearPreview();
        return;
    }

    TArray<FVector> Points;
    Points.Reserve(AxialPath.Num());
    for (const auto &C : AxialPath)
        if (AHexTile *T = GM->GetHexTileAt(C))
            Points.Add(T->GetActorLocation());

    PathView->Show(Points);
}

void ADemoGameMode::ClearPreview()
{
    LastStart = {INT32_MAX, INT32_MAX};
    LastGoal = {INT32_MAX, INT32_MAX};
    if (PathView)
        PathView->Clear();
}

void ADemoGameMode::SetPreviewEnabled(bool bEnabled)
{
    bPreviewEnabled = bEnabled;
    if (!bPreviewEnabled)
        ClearPreview();
}

void ADemoGameMode::TogglePreview()
{
    bPreviewEnabled = !bPreviewEnabled;
    if (!bPreviewEnabled)
        ClearPreview();
}

void ADemoGameMode::OpenShopAt(AHexTile *ShopTile)
{
    if (!ShopTile)
        return;

    if (!ShopWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShopWidgetClass non défini"));
        return;
    }

    if (UUserWidget *W = CreateWidget<UUserWidget>(GetWorld(), ShopWidgetClass))
    {
        W->AddToViewport();
        if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            PC->bShowMouseCursor = true;
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(Mode);
        }
        ShopWidget = W;
    }
}

void ADemoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(SnapRetryHandle);
    GetWorldTimerManager().ClearTimer(PreviewThrottle);

    if (IsValid(PathView) && !PathView->IsActorBeingDestroyed())
    {
        PathView->Destroy();
        PathView = nullptr;
    }

    if (ShopWidget.IsValid())
    {
        ShopWidget->RemoveFromParent();
        ShopWidget.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void ADemoGameMode::TrySnapPawnOnce()
{
    if (!GridManager)
        return;

    APlayerController *PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
        return;

    AHexPawn *P = Cast<AHexPawn>(PC->GetPawn());
    if (!P)
        return;

    AHexTile *T = GridManager->GetHexTileAt(StartCoords);
    if (!T)
        return;

    // Snap pawn and camera
    P->SetCurrentTile(T);
    P->SetActorLocation(T->GetActorLocation());
    PC->bAutoManageActiveCameraTarget = false;
    PC->SetViewTarget(P);

    GetWorldTimerManager().ClearTimer(SnapRetryHandle);
    UpdateReachableVisibility(3);
    UE_LOG(LogTemp, Warning, TEXT("Snap OK sur (%d,%d)"), StartCoords.Q, StartCoords.R);
}

void ADemoGameMode::UpdateReachableVisibility(int32 MaxSteps)
{
    if (!GridManager)
        return;

    AHexPawn *P = GetPlayerPawnTyped();
    if (!P)
        return;

    AHexTile *StartTile = P->GetCurrentTile();
    if (!StartTile)
        return;

    const FHexAxialCoordinates Start = StartTile->GetAxialCoordinates();

    // BFS borné en nombre de pas
    TSet<FHexAxialCoordinates> Visited;
    TMap<FHexAxialCoordinates, int32> Dist;
    TQueue<FHexAxialCoordinates> Q;

    Visited.Add(Start);
    Dist.Add(Start, 0);
    Q.Enqueue(Start);

    while (!Q.IsEmpty())
    {
        FHexAxialCoordinates Cur;
        Q.Dequeue(Cur);
        const int32 D = Dist[Cur];
        if (D >= MaxSteps)
            continue;

        const TArray<FHexAxialCoordinates> Neigh = GridManager->GetNeighbors(Cur);
        for (const FHexAxialCoordinates &N : Neigh)
        {
            if (!Visited.Contains(N))
            {
                Visited.Add(N);
                Dist.Add(N, D + 1);
                Q.Enqueue(N);
            }
        }
    }

    // Parcours de toute la grille via bornes connues
    const int32 R = GridRadius;             // déjà dans le GameMode
    for (int32 q = -2 * R; q <= 2 * R; ++q) // doubled-q
    {
        for (int32 r = -R; r <= R; ++r)
        {
            if (AHexTile *T = GridManager->GetHexTileAt({q, r}))
            {
                const bool bReachable = Visited.Contains({q, r});
                T->SetActorHiddenInGame(!bReachable);
                T->SetActorEnableCollision(bReachable);
            }
        }
    }
}

void ADemoGameMode::StartTestBattle()
{
    UE_LOG(LogTemp, Warning, TEXT("[Battle] GM=%s  CatalogSize=%d"),
           *GetClass()->GetName(), EnemyCatalog.Num());
    for (const auto &Kvp : EnemyCatalog)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Battle] Catalog key=%s -> asset=%s"),
               *Kvp.Key.ToString(), *GetNameSafe(Kvp.Value.Get()));
    }

    UBattleWidget *W = CreateWidget<UBattleWidget>(GetWorld(), BattleWidgetClass);
    if (!W)
        return;
    W->AddToViewport(20);

    AHexPawn *PlayerPawn = GetPlayerPawnTyped();
    UCombatComponent *PlayerCombat = PlayerPawn ? PlayerPawn->GetCombat() : nullptr;

    // Choose enemy id
    const FName EnemyId = PickRandomEnemyIdFromCatalog();
    if (EnemyId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("EnemyCatalog empty"));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[Battle] Picked id=%s"), *EnemyId.ToString());
    // Spawn enemy with data BEFORE BeginPlay
    FVector BaseLoc = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
    FTransform X(FRotator::ZeroRotator, BaseLoc + FVector(2000, 0, 0));

    UClass *SpawnClass = EnemyPawnClass ? EnemyPawnClass.Get() : AHexEnemyPawn::StaticClass();
    AHexEnemyPawn *EnemyPawn = GetWorld()->SpawnActorDeferred<AHexEnemyPawn>(SpawnClass, X);
    if (EnemyPawn)
    {
        if (TSoftObjectPtr<UEnemyDefinition> *Entry = EnemyCatalog.Find(EnemyId))
        {
            EnemyPawn->EnemyData = *Entry;

            if (UEnemyDefinition *Def = Entry->LoadSynchronous())
            {
                if (UTexture2D *Tex = Def->Portrait.LoadSynchronous())
                {
                    W->SetEnemyPortrait(Tex);
                }
                W->SetVictoryXP(Def->XPReward);   // <-- pass reward
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("EnemyCatalog has no entry for id=%s"), *EnemyId.ToString());
        }
        UGameplayStatics::FinishSpawningActor(EnemyPawn, X);
        APlayerController *PC = GetWorld()->GetFirstPlayerController();
        if (PC && PlayerPawn)
        {
            PC->bAutoManageActiveCameraTarget = false; // <- replace the bad call
            if (PC->GetPawn() != PlayerPawn)
                PC->Possess(PlayerPawn);
            PC->SetViewTargetWithBlend(PlayerPawn, 0.0f);
            PC->bShowMouseCursor = true;

            FInputModeGameAndUI Mode;
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(Mode);
        }
    }

    W->SetSides(PlayerCombat, EnemyPawn ? EnemyPawn->GetCombat() : nullptr);
}

void ADemoGameMode::OpenLoadoutEditor()
{
    if (!LoadoutWidgetClass)
        return;

    if (ULoadoutEditorWidget *W = CreateWidget<ULoadoutEditorWidget>(GetWorld(), LoadoutWidgetClass))
    {
        W->AddToViewport(15);
        W->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
        W->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        W->SetPositionInViewport(FVector2D(0, 0));
        W->SetDesiredSizeInViewport(FVector2D(700, 480));

        if (AHexPawn *P = GetPlayerPawnTyped())
        {
            W->SetCombat(P->GetCombat());
        }
    }
}

FName ADemoGameMode::PickRandomEnemyIdFromCatalog() const
{
    TArray<FName> Keys;
    EnemyCatalog.GetKeys(Keys);
    if (Keys.Num() == 0)
        return NAME_None;
    const int32 idx = FMath::RandHelper(Keys.Num());
    UE_LOG(LogTemp, Log, TEXT("Catalog keys: %s -> pick[%d]=%s"),
           *FString::JoinBy(Keys, TEXT(","), [](const FName &N)
                            { return N.ToString(); }),
           idx, *Keys[idx].ToString());
    return Keys[idx];
}

void ADemoGameMode::OnPawnArrived(AHexPawn* Pawn)
{
    if (!Pawn) return;
    if (AHexTile* T = Pawn->GetCurrentTile())
    {
        if (T->GetTileType() == EHexTileType::Enemy)
        {
            UE_LOG(LogTemp, Warning, TEXT("[TileEvent] Enemy tile at (%d,%d)"),
                   T->GetAxialCoordinates().Q, T->GetAxialCoordinates().R);
            StartTestBattle();
        }
    }
}