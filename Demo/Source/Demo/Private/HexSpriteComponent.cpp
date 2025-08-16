#include "HexSpriteComponent.h"
#include "Net/UnrealNetwork.h"

UHexSpriteComponent::UHexSpriteComponent()
{
    SetIsReplicatedByDefault(true);
    SetLooping(true);
}

void UHexSpriteComponent::SetAnimationState(EHexAnimState NewState)
{
    if (CurrentAnimState == NewState)
        return;
    CurrentAnimState = NewState;
    UE_LOG(LogTemp, Warning, TEXT("[SpriteComp %s] SetAnimationState -> %s | Owner=%s IsCDO=%d"),
           *GetName(),
           *UEnum::GetValueAsString(NewState),
           *GetNameSafe(GetOwner()),
           HasAnyFlags(RF_ClassDefaultObject) ? 1 : 0);
    ApplyAnim();
}

void UHexSpriteComponent::OnRep_AnimState()
{
    UE_LOG(LogTemp, Warning, TEXT("[SpriteComp %s] OnRep_AnimState -> %s"),
           *GetOwner()->GetName(),
           *UEnum::GetValueAsString(CurrentAnimState));
    ApplyAnim();
}

void UHexSpriteComponent::ApplyAnim()
{
    UPaperFlipbook *Wanted = nullptr;
    switch (CurrentAnimState)
    {
    case EHexAnimState::Walking:
        Wanted = WalkAnim;
        break;
    default:
        Wanted = IdleAnim;
        break;
    }

    if (Wanted && GetFlipbook() != Wanted)
    {
        SetFlipbook(Wanted);
        PlayFromStart();
    }
    else if (Wanted && !IsPlaying())
    {
        Play();
    }
}

void UHexSpriteComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UHexSpriteComponent, CurrentAnimState);
}
