#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InterVerseTypes.h"
#include "InterVerseVRMenuWidget.generated.h"

class UComboBoxString;
class UEditableTextBox;
class UTextBlock;

USTRUCT()
struct FInterVerseVRMenuDestination
{
    GENERATED_BODY()

    FString Id;
    FString DisplayName;
    FString NavigationAnchor;
    FString Category;
};

UCLASS()
class INTERVERSERUNTIME_API UInterVerseVRMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Menu")
    void SetStatusText(const FString& Message);

private:
    bool LoadDestinations();
    FString DisplayNameForId(const FString& Id, const FString& ExplicitDisplayName) const;
    FString CategoryForDestination(const FString& Id, const FString& DisplayName) const;
    void RebuildCategoryOptions();
    void RebuildDestinationOptions();
    const FInterVerseVRMenuDestination* FindSelectedDestination() const;
    void StartGuidance(const FString& NavigationAnchor, const FString& DisplayName);

    UFUNCTION() void HandleCategoryChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleSearchChanged(const FText& Text);
    UFUNCTION() void GuideSelectedDestination();
    UFUNCTION() void StopActiveGuidance();
    UFUNCTION() void AskGraduateSchoolAI();
    UFUNCTION() void HandleValidatedCommand(const FInterVerseValidatedCommand& Command);
    UFUNCTION() void HandleCloudError(const FString& Message);
    UFUNCTION() void CloseMenu();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> CategoryCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> DestinationCombo;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> SearchBox;

    UPROPERTY(Transient)
    TArray<FInterVerseVRMenuDestination> Destinations;

    FString ActiveCategory = TEXT("Todos");
    FString SearchFilter;
};
