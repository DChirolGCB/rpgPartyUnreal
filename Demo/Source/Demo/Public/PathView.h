#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PathView.generated.h"

class ULineBatchComponent;

UCLASS()
class DEMO_API APathView : public AActor
{
	GENERATED_BODY()

public:
	APathView();

	// Dessine des segments entre Points (ordre donné)
	UFUNCTION(BlueprintCallable, Category="PathView")
	void Show(const TArray<FVector>& Points);

	// Efface tout
	UFUNCTION(BlueprintCallable, Category="PathView")
	void Clear();

	// Couleur et épaisseur des lignes
	UPROPERTY(EditAnywhere, Category="PathView")
	FLinearColor Color = FLinearColor::Red;

	UPROPERTY(EditAnywhere, Category="PathView", meta=(ClampMin="1.0"))
	float Thickness = 6.f;

	// Décalage Z pour éviter le z-fighting
	UPROPERTY(EditAnywhere, Category="PathView")
	float ZOffset = 2.f;

	// Durée d’affichage. 0 = une frame. Mettre grand si persistant
	UPROPERTY(EditAnywhere, Category="PathView")
	float LifeTime = 1e6f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category="PathView")
	ULineBatchComponent* LineBatch = nullptr;

	// Copie Points en ajoutant le ZOffset
	void BuildPointsWithOffset(const TArray<FVector>& In, TArray<FVector>& Out) const;
};
