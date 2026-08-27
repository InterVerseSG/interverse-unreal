#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InterVerseXRPawn.generated.h"

class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UInterVerseCloudClient;
class UInterVerseNavigationComponent;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseXRPawn : public APawn
{
    GENERATED_BODY()

public:
    AInterVerseXRPawn();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|XR")
    TObjectPtr<USceneComponent> VROrigin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|XR")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|XR")
    TObjectPtr<UMotionControllerComponent> LeftController;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|XR")
    TObjectPtr<UMotionControllerComponent> RightController;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|AI")
    TObjectPtr<UInterVerseCloudClient> CloudClient;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Navigation")
    TObjectPtr<UInterVerseNavigationComponent> Navigation;
};
