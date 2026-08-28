#include "InterVerseNavigationComponent.h"

#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

UInterVerseNavigationComponent::UInterVerseNavigationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UInterVerseNavigationComponent::HasAnchorTag(const AActor* Actor, const FString& NavigationAnchor) const
{
    if (!Actor || NavigationAnchor.IsEmpty())
    {
        return false;
    }

    const FName AnchorName(*NavigationAnchor);
    return Actor->ActorHasTag(FName(TEXT("InterVerseNavAnchor"))) && Actor->ActorHasTag(AnchorName);
}

AActor* UInterVerseNavigationComponent::FindAnchorActor(const FString& NavigationAnchor) const
{
    if (!GetWorld() || NavigationAnchor.IsEmpty())
    {
        return nullptr;
    }

    TArray<AActor*> Targets;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), Targets);
    for (AActor* Target : Targets)
    {
        if (HasAnchorTag(Target, NavigationAnchor))
        {
            return Target;
        }
    }
    return nullptr;
}

bool UInterVerseNavigationComponent::NavigateToAnchor(const FString& NavigationAnchor)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        OnNavigationFinished.Broadcast(false, TEXT("InterVerse navigation component has no owner actor."));
        return false;
    }

    AActor* Anchor = FindAnchorActor(NavigationAnchor);
    if (!Anchor)
    {
        const FString Message = FString::Printf(TEXT("Navigation anchor not found: %s"), *NavigationAnchor);
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: %s"), *Message);
        OnNavigationFinished.Broadcast(false, Message);
        return false;
    }

    FVector Destination = Anchor->GetActorLocation();
    Destination.Z += DestinationZOffsetCm;
    const FRotator Rotation = bPreserveRotation ? OwnerActor->GetActorRotation() : Anchor->GetActorRotation();

    const bool bTeleported = OwnerActor->TeleportTo(Destination, Rotation, false, true);
    const FString Message = bTeleported
        ? FString::Printf(TEXT("Navigated to %s"), *NavigationAnchor)
        : FString::Printf(TEXT("Teleport failed for %s"), *NavigationAnchor);

    if (bTeleported)
    {
        StopGuidance();
    }

    UE_LOG(LogTemp, Log, TEXT("InterVerseSG: %s"), *Message);
    OnNavigationFinished.Broadcast(bTeleported, Message);
    return bTeleported;
}

bool UInterVerseNavigationComponent::ExecuteValidatedCommand(const FInterVerseValidatedCommand& Command)
{
    if (!Command.bAccepted)
    {
        OnNavigationFinished.Broadcast(false, Command.Message.IsEmpty() ? TEXT("Command was not accepted by InterVerse Builder.") : Command.Message);
        return false;
    }

    if (!Command.Action.Equals(TEXT("navigate"), ESearchCase::IgnoreCase))
    {
        const FString Message = FString::Printf(TEXT("Command action '%s' is not handled by the navigation component."), *Command.Action);
        OnNavigationFinished.Broadcast(false, Message);
        return false;
    }

    if (Command.NavigationAnchor.IsEmpty())
    {
        OnNavigationFinished.Broadcast(false, TEXT("Validated navigate command has no navigation_anchor."));
        return false;
    }

    return NavigateToAnchor(Command.NavigationAnchor);
}

bool UInterVerseNavigationComponent::BuildGuidancePath(const FVector& Start, const FVector& End)
{
    GuidancePathPoints.Reset();
    GuidancePointIndex = 0;
    bGuidanceUsingNavMesh = false;

    if (!bUseNavMeshGuidance || !GetWorld())
    {
        return false;
    }

    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        GetWorld(), Start, End, GetOwner()
    );
    if (!Path || !Path->IsValid() || Path->PathPoints.Num() < 2)
    {
        return false;
    }

    GuidancePathPoints = Path->PathPoints;
    GuidancePointIndex = 1;
    bGuidanceUsingNavMesh = true;
    return true;
}

float UInterVerseNavigationComponent::RemainingPathDistanceFrom(const FVector& CurrentLocation) const
{
    if (!bGuidanceUsingNavMesh || GuidancePathPoints.Num() < 2 || !GuidancePathPoints.IsValidIndex(GuidancePointIndex))
    {
        return 0.0f;
    }

    float Total = FVector::Distance(CurrentLocation, GuidancePathPoints[GuidancePointIndex]);
    for (int32 Index = GuidancePointIndex; Index < GuidancePathPoints.Num() - 1; ++Index)
    {
        Total += FVector::Distance(GuidancePathPoints[Index], GuidancePathPoints[Index + 1]);
    }
    return Total;
}

bool UInterVerseNavigationComponent::StartGuidanceToAnchor(const FString& NavigationAnchor)
{
    AActor* OwnerActor = GetOwner();
    AActor* Anchor = FindAnchorActor(NavigationAnchor);
    if (!OwnerActor || !Anchor)
    {
        const FString Message = FString::Printf(TEXT("Guidance anchor not found: %s"), *NavigationAnchor);
        OnNavigationFinished.Broadcast(false, Message);
        return false;
    }

    GuidanceAnchor = NavigationAnchor;
    bGuidanceActive = true;
    BuildGuidancePath(OwnerActor->GetActorLocation(), Anchor->GetActorLocation());
    return UpdateGuidance();
}

void UInterVerseNavigationComponent::StopGuidance()
{
    bGuidanceActive = false;
    bGuidanceUsingNavMesh = false;
    GuidanceAnchor.Reset();
    GuidanceDirection = FVector::ZeroVector;
    GuidanceDistanceCm = 0.0f;
    GuidancePathPoints.Reset();
    GuidancePointIndex = 0;
}

bool UInterVerseNavigationComponent::UpdateGuidance()
{
    AActor* OwnerActor = GetOwner();
    if (!bGuidanceActive || !OwnerActor || GuidanceAnchor.IsEmpty())
    {
        return false;
    }

    AActor* Anchor = FindAnchorActor(GuidanceAnchor);
    if (!Anchor)
    {
        StopGuidance();
        return false;
    }

    const FVector CurrentLocation = OwnerActor->GetActorLocation();

    if (bGuidanceUsingNavMesh && GuidancePathPoints.IsValidIndex(GuidancePointIndex))
    {
        while (GuidancePathPoints.IsValidIndex(GuidancePointIndex)
            && FVector::Distance(CurrentLocation, GuidancePathPoints[GuidancePointIndex]) <= GuidanceWaypointAcceptanceCm
            && GuidancePointIndex < GuidancePathPoints.Num() - 1)
        {
            ++GuidancePointIndex;
        }

        if (GuidancePathPoints.IsValidIndex(GuidancePointIndex))
        {
            FVector Delta = GuidancePathPoints[GuidancePointIndex] - CurrentLocation;
            GuidanceDirection = Delta.GetSafeNormal();
            GuidanceDistanceCm = RemainingPathDistanceFrom(CurrentLocation);
            OnGuidanceUpdated.Broadcast(GuidanceAnchor, GuidanceDirection, GuidanceDistanceCm);
            return true;
        }
    }

    FVector Delta = Anchor->GetActorLocation() - CurrentLocation;
    GuidanceDistanceCm = Delta.Size();
    GuidanceDirection = Delta.GetSafeNormal();
    bGuidanceUsingNavMesh = false;
    OnGuidanceUpdated.Broadcast(GuidanceAnchor, GuidanceDirection, GuidanceDistanceCm);
    return true;
}
