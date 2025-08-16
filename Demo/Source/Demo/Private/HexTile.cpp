#include "HexTile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DemoGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"

AHexTile::AHexTile()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;

    PrimaryActorTick.bCanEverTick = true; // ← indispensable
    SetActorTickEnabled(false);           // on ticke seulement pendant l’anim
}

void AHexTile::BeginPlay()
{
    Super::BeginPlay();

    // Init élévation AVANT tout early-return
    BaseZ   = GetActorLocation().Z;
    TargetZ = BaseZ;
    bElevInterpActive = false;

    if (!GetVisualMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("HexTile: aucun mesh visuel trouvé sur %s"), *GetName());
        return;
    }

    if (UStaticMeshComponent* Mesh = GetVisualMesh())
    {
        // S’assure qu’on peut bouger visuellement la tuile
        if (Mesh->Mobility != EComponentMobility::Movable)
            Mesh->SetMobility(EComponentMobility::Movable);

        if (!DynamicMaterial)
            DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);

        if (DynamicMaterial)
        {
            const float V = bIsHighlighted ? 1.0f : 0.0f;
            DynamicMaterial->SetScalarParameterValue(TEXT("IsHighlighted"), V);
            UpdateMaterialColor();
        }
    }

    if (DynamicMaterial && TileType == EHexTileType::Shop)
    {
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TypeTint_Shop);
        DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), TypeTint_Shop);
        DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.5f);
    }
}

void AHexTile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bElevInterpActive)
        return;

    FVector Loc = GetActorLocation();
    const float NewZ = FMath::FInterpTo(Loc.Z, TargetZ, DeltaSeconds, HighlightLerpSpeed);
    Loc.Z = NewZ;
    SetActorLocation(Loc);

    if (FMath::IsNearlyEqual(NewZ, TargetZ, 0.5f))
    {
        Loc.Z = TargetZ;
        SetActorLocation(Loc);
        bElevInterpActive = false;
        SetActorTickEnabled(false);
    }
}



void AHexTile::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    OnClicked.AddDynamic(this, &AHexTile::HandleOnClicked);
    OnBeginCursorOver.AddDynamic(this, &AHexTile::HandleOnBeginCursorOver);
    OnEndCursorOver.AddDynamic(this, &AHexTile::HandleOnEndCursorOver);
}

UStaticMeshComponent* AHexTile::GetVisualMesh()
{
    if (IsValid(CachedVisualMesh) && !CachedVisualMesh->IsBeingDestroyed())
        return CachedVisualMesh;

    if (IsValid(TileMesh))
        return CachedVisualMesh = TileMesh;

    TArray<UStaticMeshComponent*> comps;
    GetComponents<UStaticMeshComponent>(comps);

    if (VisualMeshTag != NAME_None)
        for (auto* c : comps)
            if (IsValid(c) && c->ComponentHasTag(VisualMeshTag))
                return CachedVisualMesh = c;

    const FString Wanted = VisualMeshName.ToString();
    for (auto* c : comps)
        if (IsValid(c) && c->GetName().Equals(Wanted, ESearchCase::CaseSensitive))
            return CachedVisualMesh = c;

    return CachedVisualMesh = (comps.Num() ? comps[0] : nullptr);
}

void AHexTile::HandleOnClicked(AActor* /*TouchedActor*/, FKey /*ButtonPressed*/)
{
    // Si c'est une case Shop => ouvrir la boutique et NE PAS lancer le pathfinding
    if (TileType == EHexTileType::Shop || bIsShop || ActorHasTag(TEXT("Shop")))
    {
        if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
            GM->OpenShopAt(this);
        return;
    }

    // Sinon comportement normal (click-to-move)
    if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->HandleTileClicked(this);
    }
}

void AHexTile::HandleOnBeginCursorOver(AActor* /*TouchedActor*/)
{
    SetHighlighted(true);

    if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->PreviewPathTo(this);
    }

    // Pour le contour, on agit sur le mesh (et pas sur l’acteur)
    if (TileType == EHexTileType::Shop || bIsShop)
        if (UStaticMeshComponent* Mesh = GetVisualMesh())
            Mesh->SetRenderCustomDepth(true);
}

void AHexTile::HandleOnEndCursorOver(AActor* /*TouchedActor*/)
{
    SetHighlighted(false);

    if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->ClearPreview();
    }

    if (TileType == EHexTileType::Shop || bIsShop)
        if (UStaticMeshComponent* Mesh = GetVisualMesh())
            Mesh->SetRenderCustomDepth(false);
}

void AHexTile::SetHighlighted(bool bHighlight)
{
    if (bIsHighlighted == bHighlight)
        return;

    bIsHighlighted = bHighlight;

    if (UStaticMeshComponent* Mesh = GetVisualMesh())
    {
        if (!DynamicMaterial)
            DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);

        if (DynamicMaterial)
        {
            DynamicMaterial->SetScalarParameterValue(TEXT("IsHighlighted"), bIsHighlighted ? 1.0f : 0.0f);
            DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), bIsHighlighted ? GlowStrengthOn : GlowStrengthOff);
            DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), GlowColor);
            UpdateMaterialColor();
        }

        Mesh->SetRenderCustomDepth(bIsHighlighted);
        Mesh->SetCustomDepthStencilValue(1);
    }

    TargetZ = bIsHighlighted ? (BaseZ + HighlightLiftZ) : BaseZ;
    bElevInterpActive = true;
    SetActorTickEnabled(true);
}


void AHexTile::UpdateMaterialColor()
{
    if (!DynamicMaterial)
        return;

    const FLinearColor ColorToUse = bIsHighlighted ? HighlightColor : NormalColor;

    DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), ColorToUse);
    DynamicMaterial->SetVectorParameterValue(TEXT("Color"), ColorToUse);
    DynamicMaterial->SetVectorParameterValue(TEXT("Albedo"), ColorToUse);
    DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), ColorToUse.A);
    DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), GlowColor);
    DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), bIsHighlighted ? GlowStrengthOn : GlowStrengthOff);
}

void AHexTile::SetTileType(EHexTileType NewType)
{
    TileType = NewType;

    if (!DynamicMaterial)
        if (UStaticMeshComponent* Mesh = GetVisualMesh())
            DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);

    if (DynamicMaterial && TileType == EHexTileType::Shop)
    {
        DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.1f, 1.f, 0.1f, 1.f));
        DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.5f);
    }
}
