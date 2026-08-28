#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InterVerseTypes.h"
#include "InterVerseNavigationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInterVerseNavigationResult, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInterVerseGuidanceUpdated, const FString&, NavigationAnchor, FVector, Direction, float, DistanceCm);

UCLASS(ClassGroup=(InterVerse), meta=(BlueprintSpawnableComponent))
class INTERVERSERUNTIME_API UInterVerseNavigationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInterVerseNavigationComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Navigation")
    float DestinationZOffsetCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Navigation")
    bool bPreserveRotation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Navigation|Guidance")
    bool bUseNavMeshGuidance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Navigation|Guidance", meta=(ClampMin="50.0"))
    float GuidanceWaypointAcceptanceCm = 180.0f;

    UPROPERTY(BlueprintAssignable, Category="InterVerse|Navigation")
    FInterVerseNavigationResult OnNavigationFinished;

    UPROPERTY(BlueprintAssignable, Category="InterVerse|Navigation|Guidance")
    FInterVerseGuidanceUpdated OnGuidanceUpdated;

    UFUNCTION(BlueprintCallable, Category="InterVerse|Navigation")
    bool NavigateToAnchor(const FString& NavigationAnchor);

    UFUNCTION(BlueprintCallable, Category="InterVerse|Navigation")
    bool ExecuteValidatedCommand(const FInterVerseValidatedCommand& Command);

    UFUNCTION(BlueprintPure, Category="InterVerse|Navigation")
    AActor* FindAnchorActor(const FString& NavigationAnchor) const;

    UFUNCTION(BlueprintCallable, Category="InterVerse|Navigation|Guidance")
    bool StartGuidanceToAnchor(const FString& NavigationAnchor);

    UFUNCTION(BlueprintCallable, Category="InterVerse|Navigation|Guidance")
    void StopGuidance();

    UFUNCTION(BlueprintCallable, Category="InterVerse|Navigation|Guidance")
    bool UpdateGuidance();

    UFUNCTION(BlueprintPure, Category="InterVerse|Navigation|Guidance")
    bool IsGuidanceActive() const { return bGuidanceActive; }

    UFUNCTION(BlueprintPure, Category="InterVerse|Navigation|Guidance")
    FString GetGuidanceAnchor() const { return GuidanceAnchor; }

    UFUNCTION(BlueprintPure, Category="InterVerse|Navigation|Guidance")
    float GetGuidanceDistanceCm() const { return GuidanceDistanceCm; }

    UFUNCTION(BlueprintPure, Category="InterVerse|Navigation|Guidance")
    FVector GetGuidanceDirection() const { return GuidanceDirection; }

    UFUNCTION(BlueprintPure, Category="InterVerse|Navigation|Guidance")
    bool IsUsingNavMeshGuidance() const { return bGuidanceUsingNavMesh; }

private:
    bool HasAnchorTag(const AActor* Actor, const FString& NavigationAnchor) const;
    bool BuildGuidancePath(const FVector& Start, const FVector& End);
    float RemainingPathDistanceFrom(const FVector& CurrentLocation) const;

    bool bGuidanceActive = false;
    bool bGuidanceUsingNavMesh = false;
    FString GuidanceAnchor;
    FVector GuidanceDirection = FVector::ZeroVector;
    float GuidanceDistanceCm = 0.0f;
    TArray<FVector> GuidancePathPoints;
    int32 GuidancePointIndex = 0;
};
