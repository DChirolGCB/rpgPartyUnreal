#pragma once

#include "CoreMinimal.h"
#include "HexCoordinates.generated.h"

USTRUCT(BlueprintType)
struct DEMO_API FHexAxialCoordinates
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex")
    int32 Q = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex")
    int32 R = 0;

    FHexAxialCoordinates() = default;
    FHexAxialCoordinates(int32 InQ, int32 InR) : Q(InQ), R(InR) {}

    // opérateur == nécessaire pour TMap.Find(...)
    bool operator==(const FHexAxialCoordinates& Other) const
    {
        return Q == Other.Q && R == Other.R;
    }

    // distance hexagonale « manhattan » (axial coords)
    int32 DistanceTo(const FHexAxialCoordinates& Other) const
    {
        const int32 dQ = FMath::Abs(Q - Other.Q);
        const int32 dR = FMath::Abs(R - Other.R);
        const int32 dS = FMath::Abs((Q + R) - (Other.Q + Other.R));
        return (dQ + dR + dS) / 2;
    }
};

// hash pour le TMap
FORCEINLINE uint32 GetTypeHash(const FHexAxialCoordinates& Coords)
{
    return HashCombine(::GetTypeHash(Coords.Q), ::GetTypeHash(Coords.R));
}
