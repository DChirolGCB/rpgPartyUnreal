#include "HexTile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DemoGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// Material parameter names (avoids repeated FName allocations)
	static const FName PARAM_BaseColor = TEXT("BaseColor");
	static const FName PARAM_Color = TEXT("Color");
	static const FName PARAM_Albedo = TEXT("Albedo");
	static const FName PARAM_Opacity = TEXT("Opacity");
	static const FName PARAM_EmissiveColor = TEXT("EmissiveColor");
	static const FName PARAM_EmissiveStr = TEXT("EmissiveStrength");
	static const FName PARAM_IsHighlighted = TEXT("IsHighlighted");
}

AHexTile::AHexTile()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
}

/** Called when the game starts or when spawned */
void AHexTile::BeginPlay()
{
	Super::BeginPlay();

	BaseZ = GetActorLocation().Z;
	TargetZ = BaseZ;
	bElevInterpActive = false;

	UStaticMeshComponent *Mesh = GetVisualMesh();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("HexTile: no visual mesh found on %s"), *GetName());
		return;
	}

	// Ensure mesh can move visually
	if (Mesh->Mobility != EComponentMobility::Movable)
	{
		Mesh->SetMobility(EComponentMobility::Movable);
	}

	// Create MID once
	if (!DynamicMaterial)
	{
		DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	// Initialize visual parameters
	if (DynamicMaterial)
	{
		const float V = bIsHighlighted ? 1.0f : 0.0f;
		DynamicMaterial->SetScalarParameterValue(PARAM_IsHighlighted, V);
		UpdateMaterialColor();
	}

	// Optional shop tint
	if (DynamicMaterial && TileType == EHexTileType::Shop)
	{
		DynamicMaterial->SetVectorParameterValue(PARAM_BaseColor, TypeTint_Shop);
		DynamicMaterial->SetVectorParameterValue(PARAM_EmissiveColor, TypeTint_Shop);
		DynamicMaterial->SetScalarParameterValue(PARAM_EmissiveStr, 0.5f);
	}
}

/** Called every frame */
void AHexTile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bElevInterpActive)
	{
		return;
	}

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

/** Bind click and hover events */
void AHexTile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	OnClicked.AddDynamic(this, &AHexTile::HandleOnClicked);
	OnBeginCursorOver.AddDynamic(this, &AHexTile::HandleOnBeginCursorOver);
	OnEndCursorOver.AddDynamic(this, &AHexTile::HandleOnEndCursorOver);
}

/** Returns the main visual mesh of the tile (cached) */
UStaticMeshComponent *AHexTile::GetVisualMesh()
{
	if (IsValid(CachedVisualMesh) && !CachedVisualMesh->IsBeingDestroyed())
	{
		return CachedVisualMesh;
	}

	if (IsValid(TileMesh))
	{
		return CachedVisualMesh = TileMesh;
	}

	TArray<UStaticMeshComponent *> Comps;
	GetComponents<UStaticMeshComponent>(Comps);

	if (VisualMeshTag != NAME_None)
	{
		for (auto *C : Comps)
		{
			if (IsValid(C) && C->ComponentHasTag(VisualMeshTag))
			{
				return CachedVisualMesh = C;
			}
		}
	}

	const FString Wanted = VisualMeshName.ToString();
	for (auto *C : Comps)
	{
		if (IsValid(C) && C->GetName().Equals(Wanted, ESearchCase::CaseSensitive))
		{
			return CachedVisualMesh = C;
		}
	}

	return CachedVisualMesh = (Comps.Num() ? Comps[0] : nullptr);
}

/** Called when the tile is clicked */
void AHexTile::HandleOnClicked(AActor *, FKey)
{
	if (TileType == EHexTileType::Shop || bIsShop || ActorHasTag(TEXT("Shop")))
	{
		if (ADemoGameMode *GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->OpenShopAt(this);
		}
		return;
	}

	if (ADemoGameMode *GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->HandleTileClicked(this);
	}
}

/** Called when the mouse cursor enters the tile */
void AHexTile::HandleOnBeginCursorOver(AActor *)
{
	SetHighlighted(true);

	if (ADemoGameMode *GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->PreviewPathTo(this);
	}

	if (TileType == EHexTileType::Shop || bIsShop)
	{
		if (UStaticMeshComponent *Mesh = GetVisualMesh())
		{
			if (!Mesh->bRenderCustomDepth)
			{
				Mesh->SetRenderCustomDepth(true);
			}
		}
	}
}

/** Called when the mouse cursor leaves the tile */
void AHexTile::HandleOnEndCursorOver(AActor *)
{
	SetHighlighted(false);

	if (ADemoGameMode *GM = Cast<ADemoGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ClearPreview();
	}

	if (TileType == EHexTileType::Shop || bIsShop)
	{
		if (UStaticMeshComponent *Mesh = GetVisualMesh())
		{
			if (Mesh->bRenderCustomDepth)
			{
				Mesh->SetRenderCustomDepth(false);
			}
		}
	}
}

/** Changes highlight state and triggers visual effects */
void AHexTile::SetHighlighted(bool bHighlight)
{
	if (bIsHighlighted == bHighlight)
	{
		return;
	}

	bIsHighlighted = bHighlight;

	// Localize mesh once for this scope
	UStaticMeshComponent *Mesh = GetVisualMesh();

	// Material instance creation if missing
	if (Mesh && !DynamicMaterial)
	{
		DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	// Apply material highlight parameters
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(PARAM_IsHighlighted, bIsHighlighted ? 1.0f : 0.0f);
		DynamicMaterial->SetScalarParameterValue(PARAM_EmissiveStr, bIsHighlighted ? GlowStrengthOn : GlowStrengthOff);
		DynamicMaterial->SetVectorParameterValue(PARAM_EmissiveColor, GlowColor);
		UpdateMaterialColor();
	}

	// Outline via custom depth (only touch when value changes)
	if (Mesh)
	{
		if (Mesh->bRenderCustomDepth != bIsHighlighted)
		{
			Mesh->SetRenderCustomDepth(bIsHighlighted);
		}
		Mesh->SetCustomDepthStencilValue(1);
	}

	// Elevation animation
	TargetZ = bIsHighlighted ? (BaseZ + HighlightLiftZ) : BaseZ;
	bElevInterpActive = true;
	SetActorTickEnabled(true);
}

/** Updates the material color based on highlight state */
void AHexTile::UpdateMaterialColor()
{
	if (!DynamicMaterial)
	{
		return;
	}

	const FLinearColor ColorToUse = bIsHighlighted ? HighlightColor : NormalColor;

	DynamicMaterial->SetVectorParameterValue(PARAM_BaseColor, ColorToUse);
	DynamicMaterial->SetVectorParameterValue(PARAM_Color, ColorToUse);
	DynamicMaterial->SetVectorParameterValue(PARAM_Albedo, ColorToUse);
	DynamicMaterial->SetScalarParameterValue(PARAM_Opacity, ColorToUse.A);
	DynamicMaterial->SetVectorParameterValue(PARAM_EmissiveColor, GlowColor);
	DynamicMaterial->SetScalarParameterValue(PARAM_EmissiveStr, bIsHighlighted ? GlowStrengthOn : GlowStrengthOff);
}

/** Sets the tile type and adjusts visuals if necessary */
void AHexTile::SetTileType(EHexTileType NewType)
{
	TileType = NewType;

	UStaticMeshComponent *Mesh = GetVisualMesh();

	if (Mesh && !DynamicMaterial)
	{
		DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (DynamicMaterial && TileType == EHexTileType::Shop)
	{
		DynamicMaterial->SetVectorParameterValue(PARAM_EmissiveColor, FLinearColor(0.1f, 1.f, 0.1f, 1.f));
		DynamicMaterial->SetScalarParameterValue(PARAM_EmissiveStr, 0.5f);
	}
}
