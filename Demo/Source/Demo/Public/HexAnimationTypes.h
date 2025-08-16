// Public/HexAnimationTypes.h
#pragma once

#include "CoreMinimal.h"
#include "HexAnimationTypes.generated.h"

// Animation states enum (shared across the project)
UENUM(BlueprintType)
enum class EHexAnimState : uint8
{
    Idle        = 0 UMETA(DisplayName="Idle"),
    Walking     = 1 UMETA(DisplayName="Walking"),
    Attacking   = 2 UMETA(DisplayName="Attacking"),
    Damaged     = 3 UMETA(DisplayName="Damaged"),
    Dead        = 4 UMETA(DisplayName="Dead"),
    Interacting = 5 UMETA(DisplayName="Interacting"),
    Casting     = 6 UMETA(DisplayName="Casting"),
    MAX         = 7 UMETA(Hidden)
};

// Compressed state for network replication
USTRUCT(BlueprintType)
struct DEMO_API FCompressedAnimState
{
    GENERATED_BODY()

    uint8 AnimState : 3;      // 0-7 states
    uint8 FacingDir : 3;      // 0-7 directions  
    uint8 IsMoving : 1;       // Moving flag
    uint8 IsCombat : 1;       // Combat flag

    FCompressedAnimState()
    {
        AnimState = 0;
        FacingDir = 0;
        IsMoving = 0;
        IsCombat = 0;
    }

    // Helper functions
    void SetAnimState(EHexAnimState State) 
    { 
        AnimState = (uint8)State; 
    }
    
    EHexAnimState GetAnimState() const 
    { 
        return (EHexAnimState)AnimState; 
    }

    // Network serialization
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        uint8 Packed = (AnimState << 5) | (FacingDir << 2) | (IsMoving << 1) | IsCombat;
        Ar.SerializeBits(&Packed, 8);
        if (Ar.IsLoading())
        {
            AnimState = (Packed >> 5) & 0x07;
            FacingDir = (Packed >> 2) & 0x07;
            IsMoving = (Packed >> 1) & 0x01;
            IsCombat = Packed & 0x01;
        }
        bOutSuccess = true;
        return true;
    }
};

// Must tell Unreal this struct can be serialized
template<>
struct TStructOpsTypeTraits<FCompressedAnimState> : public TStructOpsTypeTraitsBase2<FCompressedAnimState>
{
    enum
    {
        WithNetSerializer = true,
    };
};

// Full animation state (for local use, not compressed)
USTRUCT(BlueprintType)
struct DEMO_API FHexAnimationState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category="Animation")
    EHexAnimState CurrentState = EHexAnimState::Idle;

    UPROPERTY(BlueprintReadWrite, Category="Animation")
    float StateStartTime = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category="Animation")
    int32 FacingDirection = 0;

    UPROPERTY(BlueprintReadWrite, Category="Animation")
    bool bIsMoving = false;

    // Convert to compressed for network
    FCompressedAnimState Compress() const
    {
        FCompressedAnimState Compressed;
        Compressed.SetAnimState(CurrentState);
        Compressed.FacingDir = FMath::Clamp(FacingDirection, 0, 7);
        Compressed.IsMoving = bIsMoving ? 1 : 0;
        Compressed.IsCombat = (CurrentState == EHexAnimState::Attacking) ? 1 : 0;
        return Compressed;
    }
};