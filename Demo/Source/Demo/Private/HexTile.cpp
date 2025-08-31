// HexTile.cpp
#include "HexTile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DemoGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    const FName PARAM_BaseColor     = TEXT("BaseColor");
    const FName PARAM_Color         = TEXT("Color");
    const FName PARAM_Albedo        = TEXT("Albedo");
    const FName PARAM_Opacity       = TEXT("Opacity");
    const FName PARAM_EmissiveColor = TEXT("EmissiveColor");
    const FName PARAM_EmissiveStr   = TEXT("EmissiveStrength");
    const FName PARAM_IsHighlighted = TEXT("IsHighlighted");
}

AHexTile::AHexTile()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;

    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);
}

void AHexTile::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    OnClicked.AddDynamic(this, &AHexTile::HandleOnClicked);
    OnBeginCursorOver.AddDynamic(this, &AHexTile::HandleOnBeginCursorOver);
    OnEndCursorOver.AddDynamic(this, &AHexTile::HandleOnEndCursorOver);
}

void AHexTile::BeginPlay()
{
    Super::BeginPlay();

    BaseZ = GetActorLocation().Z;
    TargetZ = BaseZ;
    bElevInterpActive = false;

    if (UStaticMeshComponent* Mesh = GetVisualMesh())
    {
        if (Mesh->Mobility != EComponentMobility::Movable)
            Mesh->SetMobility(EComponentMobility::Movable);

        if (!DynamicMaterial)
            DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);

        if (DynamicMaterial)
        {
            DynamicMaterial->SetScalarParameterValue(PARAM_IsHighlighted, bIsHighlighted ? 1.0f : 0.0f);
            UpdateMaterialColor();

            if (TileType == EHexTileType::Shop)
            {
                DynamicMaterial->SetVectorParameterValue(PARAM_BaseColor,     TypeTint_Shop);
                DynamicMaterial->SetVectorParameterValue(PARAM_EmissiveColor, TypeTint_Shop);
                DynamicMaterial->SetScalarParameterValue(PARAM_EmissiveStr,   0.5f);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("HexTile: no visual mesh found on %s"), *GetName());
    }
}

void AHexTile::OnConstruction(const FTransform& Xform)
{
    Super::OnConstruction(Xform);
    ApplyMaterialForType();
}

void AHexTile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bElevInterpActive) return;

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

UStaticMeshComponent* AHexTile::GetVisualMesh()
{
    if (IsValid(CachedVisualMesh) && !CachedVisualMesh->IsBeingDestroyed())
        return CachedVisualMesh;

    if (IsValid(TileMesh))
        return CachedVisualMesh = TileMesh;

    TArray<UStaticMeshComponent*> Comps;
    GetComponents<UStaticMeshComponent>(Comps);

    if (VisualMeshTag != NAME_None)
    {
        for (auto* C : Comps)
            if (IsValid(C) && C->ComponentHasTag(VisualMeshTag))
                return CachedVisualMesh = C;
    }

    const FString Wanted = VisualMeshName.ToString();
    for (auto* C : Comps)
        if (IsValid(C) && C->GetName().Equals(Wanted, ESearchCase::CaseSensitive))
            return CachedVisualMesh = C;

    return CachedVisualMesh = (Comps.Num() ? Comps[0] : nullptr);
}

void AHexTile::HandleOnClicked(AActor*, FKey)
{
    if (TileType == EHexTileType::Shop || bIsShop || ActorHasTag(TEXT("Shop")))
    {
        if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
            GM->OpenShopAt(this);
        return;
    }

    if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
        GM->HandleTileClicked(this);
}

void AHexTile::HandleOnBeginCursorOver(AActor*)
{
    SetHighlighted(true);

    if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
        GM->PreviewPathTo(this);

    if (TileType == EHexTileType::Shop || bIsShop)
        if (UStaticMeshComponent* Mesh = GetVisualMesh())
            if (!Mesh->bRenderCustomDepth)
                Mesh->SetRenderCustomDepth(true);
}

void AHexTile::HandleOnEndCursorOver(AActor*)
{
    SetHighlighted(false);

    if (ADemoGameMode* GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
        GM->ClearPreview();

    if (TileType == EHexTileType::Shop || bIsShop)
        if (UStaticMeshComponent* Mesh = GetVisualMesh())
            if (Mesh->bRenderCustomDepth)
                Mesh->SetRenderCustomDepth(false);
}

void AHexTile::SetHighlighted(bool bHighlight)
{
    if (bIsHighlighted == bHighlight) return;
    bIsHighlighted = bHighlight;

    UStaticMeshComponent* Mesh = GetVisualMesh();
    if (Mesh && !DynamicMaterial)
        DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);

    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(PARAM_IsHighlighted, bIsHighlighted ? 1.0f : 0.0f);
        DynamicMaterial->SetScalarParameterValue(PARAM_EmissiveStr,  bIsHighlighted ? GlowStrengthOn : GlowStrengthOff);
        DynamicMaterial->SetVectorParameterValue(PARAM_EmissiveColor, GlowColor);
        UpdateMaterialColor();
    }

    if (Mesh)
    {
        if (Mesh->bRenderCustomDepth != bIsHighlighted)
            Mesh->SetRenderCustomDepth(bIsHighlighted);
        Mesh->SetCustomDepthStencilValue(1);
    }

    TargetZ = bIsHighlighted ? (BaseZ + HighlightLiftZ) : BaseZ;
    bElevInterpActive = true;
    SetActorTickEnabled(true);
}

void AHexTile::UpdateMaterialColor()
{
    if (!DynamicMaterial) return;

    const FLinearColor ColorToUse = bIsHighlighted ? HighlightColor : NormalColor;
    DynamicMaterial->SetVectorParameterValue(PARAM_BaseColor,     ColorToUse);
    DynamicMaterial->SetVectorParameterValue(PARAM_Color,         ColorToUse);
    DynamicMaterial->SetVectorParameterValue(PARAM_Albedo,        ColorToUse);
    DynamicMaterial->SetScalarParameterValue(PARAM_Opacity,       ColorToUse.A);
    DynamicMaterial->SetVectorParameterValue(PARAM_EmissiveColor, GlowColor);
    DynamicMaterial->SetScalarParameterValue(PARAM_EmissiveStr,   bIsHighlighted ? GlowStrengthOn : GlowStrengthOff);
}

void AHexTile::ApplyMaterialForType()
{
    UStaticMeshComponent* Mesh = GetVisualMesh();
    if (!Mesh) return;

    UMaterialInterface* UseMat = nullptr;
    switch (TileType)
    {
        case EHexTileType::Enemy:
            UseMat = MatEnemy.IsValid() ? MatEnemy.Get() : MatEnemy.LoadSynchronous();
            break;
        default: // Normal / Shop / Spawn / Goal
            UseMat = MatNormal.IsValid() ? MatNormal.Get() : MatNormal.LoadSynchronous();
            break;
    }

    if (UseMat) Mesh->SetMaterial(0, UseMat);
}

void AHexTile::SetTileType(EHexTileType NewType)
{
    TileType = NewType;
    ApplyMaterialForType();
}
