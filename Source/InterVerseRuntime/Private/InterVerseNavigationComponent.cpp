#include "InterVerseNavigationComponent.h"

#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"

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
