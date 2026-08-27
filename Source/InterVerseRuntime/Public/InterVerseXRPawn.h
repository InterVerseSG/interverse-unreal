#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InterVerseXRPawn.generated.h"

class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UInterVerseCloudClient;
class UInterVerseNavigationComponent;
class UInterVerseVRLocomotionComponent;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseXRPawn : public APawn
{
    GENERATED_BODY()

public:
    AInterVerseXRPawn();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Tick(float DeltaSeconds) override;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR")
    TObjectPtr<UInterVerseVRLocomotionComponent> Locomotion;

private:
    void InputMoveForward(float Value);
    void InputMoveRight(float Value);
    void InputTurn(float Value);
    void InputTeleportPressed();
    void InputTeleportReleased();

    float MoveForwardValue = 0.0f;
    float MoveRightValue = 0.0f;
    bool bTurnLatched = false;
};
