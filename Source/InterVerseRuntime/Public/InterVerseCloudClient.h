#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InterVerseTypes.h"
#include "InterVerseCloudClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterVerseCommandReceived, const FInterVerseCommand&, Command);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterVerseCommandValidated, const FInterVerseValidatedCommand&, Command);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterVerseCloudError, const FString&, Message);

UCLASS(ClassGroup=(InterVerse), meta=(BlueprintSpawnableComponent))
class INTERVERSERUNTIME_API UInterVerseCloudClient : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="InterVerse|Cloud")
    FInterVerseCommandReceived OnAssistantCommand;

    UPROPERTY(BlueprintAssignable, Category="InterVerse|Cloud")
    FInterVerseCommandValidated OnCommandValidated;

    UPROPERTY(BlueprintAssignable, Category="InterVerse|Cloud")
    FInterVerseCloudError OnCloudError;

    /** Automatically send Gemini's command to InterVerse Builder for validation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Cloud")
    bool bAutoValidateAssistantCommands = true;

    /** Automatically execute accepted navigate commands if the owner has an InterVerseNavigationComponent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Cloud")
    bool bAutoExecuteValidatedNavigation = true;

    UFUNCTION(BlueprintCallable, Category="InterVerse|Cloud")
    void AskAssistant(const FInterVerseAssistantRequest& Request);

    UFUNCTION(BlueprintCallable, Category="InterVerse|Cloud")
    void ValidateCommand(const FInterVerseCommand& Command, bool bUserConfirmed = false);

private:
    FString JoinUrl(const FString& Base, const FString& Path) const;
    void TryAutoExecute(const FInterVerseValidatedCommand& Command);
};
