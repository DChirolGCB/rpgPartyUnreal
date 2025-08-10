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

    if (!GetVisualMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("HexTile: aucun mesh visuel trouvé sur %s"), *GetName());
        return;
    }

    if (UStaticMeshComponent *Mesh = GetVisualMesh())
    {
        if (!DynamicMaterial)
            DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);

        if (DynamicMaterial)
        {
            const float V = bIsHighlighted ? 1.0f : 0.0f;
            DynamicMaterial->SetScalarParameterValue(TEXT("IsHighlighted"), V);
            UpdateMaterialColor();
        }
    }
}

UStaticMeshComponent *AHexTile::GetVisualMesh()
{
    if (IsValid(CachedVisualMesh) && !CachedVisualMesh->IsBeingDestroyed())
        return CachedVisualMesh;

    if (IsValid(TileMesh))
        return CachedVisualMesh = TileMesh;

    TArray<UStaticMeshComponent *> comps;
    GetComponents<UStaticMeshComponent>(comps);

    if (VisualMeshTag != NAME_None)
        for (auto *c : comps)
            if (IsValid(c) && c->ComponentHasTag(VisualMeshTag))
                return CachedVisualMesh = c;

    const FString Wanted = VisualMeshName.ToString();
    for (auto *c : comps)
        if (IsValid(c) && c->GetName().Equals(Wanted, ESearchCase::CaseSensitive))
            return CachedVisualMesh = c;

    return CachedVisualMesh = (comps.Num() ? comps[0] : nullptr);
}

void AHexTile::HandleOnClicked(AActor *TouchedActor, FKey ButtonPressed)
{
    if (ADemoGameMode *GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->HandleTileClicked(this);
    }
}

void AHexTile::HandleOnBeginCursorOver(AActor *TouchedActor)
{
    SetHighlighted(true);

    if (ADemoGameMode *GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->PreviewPathTo(this);
    }
}

void AHexTile::HandleOnEndCursorOver(AActor *TouchedActor)
{
    SetHighlighted(false);

    if (ADemoGameMode *GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GM->ClearPreview();
    }
}

void AHexTile::SetHighlighted(bool bHighlight)
{
    bIsHighlighted = bHighlight;

    if (UStaticMeshComponent *Mesh = GetVisualMesh())
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
        Mesh->CustomDepthStencilValue = 1;
    }
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