#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatsWidget.generated.h"

class UTextBlock;
class UCombatComponent;

UCLASS()
class DEMO_API UPlayerStatsWidget : public UUserWidget
{
    GENERATED_BODY()

public: // binding
	UPlayerStatsWidget(const FObjectInitializer& Obj);

    UFUNCTION(BlueprintCallable, Category="Combat")
    void SetCombat(UCombatComponent* InCombat) { Combat = InCombat; RefreshTexts(); }

protected: // UUserWidget
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private: // helpers
    void RefreshTexts();
    FTimerHandle RefreshTimer;

private: // data source
    UPROPERTY()
    UCombatComponent* Combat = nullptr;

public: // BindWidget references (create in BP)
    UPROPERTY(meta=(BindWidget)) UTextBlock* HPText = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock* AtkText = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock* DefText = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock* XpText = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock* LvlText = nullptr;
};
