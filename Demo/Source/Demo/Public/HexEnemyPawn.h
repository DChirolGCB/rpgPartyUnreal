#pragma once
#include "CoreMinimal.h"
#include "HexPawn.h"
#include "HexEnemyPawn.generated.h"

class UEnemyDefinition;

UCLASS()
class DEMO_API AHexEnemyPawn : public AHexPawn
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
    TSoftObjectPtr<UEnemyDefinition> EnemyData;

protected:
    virtual void BeginPlay() override;
	AHexEnemyPawn();
};
