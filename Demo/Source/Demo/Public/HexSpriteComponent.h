#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbookComponent.h"
#include "HexAnimationTypes.h"
#include "PaperFlipbook.h" // en haut
#include "HexSpriteComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEMO_API UHexSpriteComponent : public UPaperFlipbookComponent
{
    GENERATED_BODY()
public:
    UHexSpriteComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hex|Anim")
    UPaperFlipbook* IdleAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hex|Anim")
    UPaperFlipbook* WalkAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_AnimState, Category="Hex|Anim")
    EHexAnimState CurrentAnimState = EHexAnimState::Idle;

    UFUNCTION(BlueprintCallable, Category="Hex|Anim")
    void SetAnimationState(EHexAnimState NewState);

protected:
    UFUNCTION()
    void OnRep_AnimState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void ApplyAnim(); // choisit le bon flipbook selon CurrentAnimState
};
