#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InterVerseTypes.h"
#include "InterVerseNavigationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInterVerseNavigationResult, bool, bSuccess, const FString&, Message);

UCLASS(ClassGroup=(InterVerse), meta=(BlueprintSpawnableComponent))
class INTERVERSERUNTIME_API UInterVerseNavigationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInterVerseNavigationComponent();

    /** Optional vertical offset added to the NAV TargetPoint before teleporting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Navigation")
    float DestinationZOffsetCm = 0.0f;

    /** If true, preserve the controlled actor's current rotation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Navigation")
    bool bPreserveRotation = true;

    UPROPERTY(BlueprintAssignable, Category="InterVerse|Navigation")
    FInterVerseNavigationResult OnNavigationFinished;

    UFUNCTION(BlueprintCallable, Category="InterVerse|Navigation")
    bool NavigateToAnchor(const FString& NavigationAnchor);

    UFUNCTION(BlueprintCallable, Category="InterVerse|Navigation")
    bool ExecuteValidatedCommand(const FInterVerseValidatedCommand& Command);

    UFUNCTION(BlueprintPure, Category="InterVerse|Navigation")
    AActor* FindAnchorActor(const FString& NavigationAnchor) const;

private:
    bool HasAnchorTag(const AActor* Actor, const FString& NavigationAnchor) const;
};
