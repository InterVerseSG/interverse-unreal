#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InterVerseTypes.h"
#include "InterVerseVRHudWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class INTERVERSERUNTIME_API UInterVerseVRHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|HUD")
    void SetCloudStatus(const FString& Message, bool bError = false);

private:
    void BuildRuntimeWidget();
    void RefreshNavigation();
    FString FriendlyNameForAnchor(const FString& NavigationAnchor) const;
    FString DirectionText(const FVector& WorldDirection) const;

    UFUNCTION()
    void HandleValidatedCommand(const FInterVerseValidatedCommand& Command);

    UFUNCTION()
    void HandleCloudError(const FString& Message);

    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DestinationText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DistanceText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DirectionTextBlock;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CloudText;

    FString LastAnchor;
    bool bArrivalShown = false;
    float ArrivalThresholdCm = 1000.0f;
};