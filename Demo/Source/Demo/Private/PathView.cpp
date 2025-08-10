#include "PathView.h"
#include "Components/LineBatchComponent.h"

APathView::APathView()
{
	LineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("LineBatch"));
	SetRootComponent(LineBatch);
	SetActorTickEnabled(false);
}

void APathView::BeginPlay()
{
	Super::BeginPlay();
	if (LineBatch) LineBatch->Flush();
}

void APathView::BuildPointsWithOffset(const TArray<FVector>& In, TArray<FVector>& Out) const
{
	Out.Reset(In.Num());
	for (const FVector& P : In)
	{
		FVector Q = P;
		Q.Z += ZOffset;
		Out.Add(Q);
	}
}

void APathView::Clear()
{
	if (!LineBatch) return;
	LineBatch->Flush();              // supprime les lignes existantes
	LineBatch->MarkRenderStateDirty();
}

void APathView::Show(const TArray<FVector>& Points)
{
	if (!LineBatch) return;

	TArray<FVector> P;
	BuildPointsWithOffset(Points, P);

	LineBatch->Flush();
	for (int32 i = 0; i + 1 < P.Num(); ++i)
	{
		LineBatch->DrawLine(P[i], P[i+1], Color, /*DepthPriority*/0, Thickness, LifeTime);
	}
	LineBatch->MarkRenderStateDirty();
}
