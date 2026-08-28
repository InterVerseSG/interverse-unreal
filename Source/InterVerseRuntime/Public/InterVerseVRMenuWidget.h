#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InterVerseTypes.h"
#include "InterVerseVRMenuWidget.generated.h"

class UTextBlock;

UCLASS()
class INTERVERSERUNTIME_API UInterVerseVRMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Menu")
    void SetStatusText(const FString& Message);

private:
    void StartGuidance(const FString& NavigationAnchor, const FString& DisplayName);

    UFUNCTION() void GuideMarquis();
    UFUNCTION() void GuideCAI();
    UFUNCTION() void GuideStudentCenter();
    UFUNCTION() void GuideGraduateSchool();
    UFUNCTION() void AskGraduateSchoolAI();
    UFUNCTION() void HandleValidatedCommand(const FInterVerseValidatedCommand& Command);
    UFUNCTION() void HandleCloudError(const FString& Message);
    UFUNCTION() void CloseMenu();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;
};
