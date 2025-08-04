#pragma once

#include "CoreMinimal.h"

#include "HexCoordinates.generated.h"

USTRUCT(BlueprintType)
struct DEMO_API FHexAxialCoordinates
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Q;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 R;

    FHexAxialCoordinates()
    {
        Q = 0;
        R = 0;
    }
    
    FHexAxialCoordinates(int32 InQ, int32 InR)
    {
        Q = InQ;
        R = InR;
    }
    
    FVector GetCubeCoordinates() const
    {
        return FVector(Q, R, -Q - R);
    }
    
    int32 DistanceTo(const FHexAxialCoordinates& Other) const
    {
        FVector CubeA = GetCubeCoordinates();
        FVector CubeB = Other.GetCubeCoordinates();
        return (FMath::Abs(CubeA.X - CubeB.X) + FMath::Abs(CubeA.Y - CubeB.Y) + FMath::Abs(CubeA.Z - CubeB.Z)) / 2;
    }
    
    bool operator==(const FHexAxialCoordinates& Other) const
    {
        return Q == Other.Q && R == Other.R;
    }
};

FORCEINLINE uint32 GetTypeHash(const FHexAxialCoordinates& Coords)
{
    return HashCombine(GetTypeHash(Coords.Q), GetTypeHash(Coords.R));
}