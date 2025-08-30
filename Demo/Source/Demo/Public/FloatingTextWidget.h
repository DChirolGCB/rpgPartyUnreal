#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingTextWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

UCLASS()
class DEMO_API UFloatingTextWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(meta=(BindWidget)) UTextBlock* Label = nullptr;
    UPROPERTY(Transient, meta=(BindWidgetAnim)) UWidgetAnimation* Pop = nullptr;

    UFUNCTION(BlueprintCallable) void SetTextAndColor(const FText& T, const FLinearColor& C);
    void PlayAndDie();

private:
    UFUNCTION() void OnAnimFinished();
};
