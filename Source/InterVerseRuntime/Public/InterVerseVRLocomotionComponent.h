#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InterVerseVRLocomotionComponent.generated.h"

class AInterVerseXRPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInterVerseTeleportAimChanged, bool, bValid, FVector, Destination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInterVerseTeleportCompleted, FVector, Destination);

UCLASS(ClassGroup=(InterVerse), meta=(BlueprintSpawnableComponent))
class INTERVERSERUNTIME_API UInterVerseVRLocomotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInterVerseVRLocomotionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Smooth Movement")
    bool bSmoothMovementEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Smooth Movement", meta=(ClampMin="10.0"))
    float SmoothMoveSpeedCmPerSecond = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Turning", meta=(ClampMin="5.0", ClampMax="90.0"))
    float SnapTurnDegrees = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Teleport")
    bool bTeleportEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Teleport", meta=(ClampMin="100.0"))
    float TeleportLaunchSpeedCmPerSecond = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Teleport", meta=(ClampMin="0.1", ClampMax="5.0"))
    float TeleportSimulationSeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Teleport", meta=(ClampMin="4", ClampMax="128"))
    int32 TeleportSimulationSteps = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Teleport")
    float TeleportDestinationLiftCm = 5.0f;

    UPROPERTY(BlueprintAssignable, Category="InterVerse|VR|Teleport")
    FInterVerseTeleportAimChanged OnTeleportAimChanged;

    UPROPERTY(BlueprintAssignable, Category="InterVerse|VR|Teleport")
    FInterVerseTeleportCompleted OnTeleportCompleted;

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR")
    void SmoothMove(FVector2D Input, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR")
    void SnapTurn(float Direction);

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Teleport")
    void BeginTeleportAim();

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Teleport")
    bool UpdateTeleportAim();

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Teleport")
    bool CommitTeleport();

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Teleport")
    void CancelTeleport();

    UFUNCTION(BlueprintPure, Category="InterVerse|VR|Teleport")
    bool IsTeleportAiming() const { return bTeleportAiming; }

    UFUNCTION(BlueprintPure, Category="InterVerse|VR|Teleport")
    bool HasValidTeleportDestination() const { return bHasValidTeleportDestination; }

    UFUNCTION(BlueprintPure, Category="InterVerse|VR|Teleport")
    FVector GetTeleportDestination() const { return TeleportDestination; }

    /** Returns sampled world-space points for the currently aimed teleport parabola. */
    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Teleport")
    void GetTeleportArcPoints(TArray<FVector>& OutPoints, bool& bOutValidDestination) const;

private:
    AInterVerseXRPawn* GetXRPawn() const;
    bool FindTeleportDestination(FVector& OutDestination) const;
    void RotatePawnAroundCamera(float DeltaYawDegrees) const;

    bool bTeleportAiming = false;
    bool bHasValidTeleportDestination = false;
    FVector TeleportDestination = FVector::ZeroVector;
};
