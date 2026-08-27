#include "InterVerseVRLocomotionComponent.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "InterVerseXRPawn.h"
#include "MotionControllerComponent.h"

UInterVerseVRLocomotionComponent::UInterVerseVRLocomotionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

AInterVerseXRPawn* UInterVerseVRLocomotionComponent::GetXRPawn() const
{
    return Cast<AInterVerseXRPawn>(GetOwner());
}

void UInterVerseVRLocomotionComponent::SmoothMove(FVector2D Input, float DeltaSeconds)
{
    if (!bSmoothMovementEnabled || Input.IsNearlyZero() || DeltaSeconds <= 0.0f)
    {
        return;
    }

    AInterVerseXRPawn* Pawn = GetXRPawn();
    if (!Pawn || !Pawn->Camera)
    {
        return;
    }

    const FRotator CameraRotation = Pawn->Camera->GetComponentRotation();
    const FRotator YawOnly(0.0f, CameraRotation.Yaw, 0.0f);
    const FVector Forward = YawOnly.Vector();
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);
    FVector Direction = Forward * Input.Y + Right * Input.X;
    Direction.Z = 0.0f;

    if (!Direction.Normalize())
    {
        return;
    }

    const FVector Delta = Direction * SmoothMoveSpeedCmPerSecond * DeltaSeconds;
    Pawn->AddActorWorldOffset(Delta, true);
}

void UInterVerseVRLocomotionComponent::RotatePawnAroundCamera(float DeltaYawDegrees) const
{
    AInterVerseXRPawn* Pawn = GetXRPawn();
    if (!Pawn || !Pawn->Camera)
    {
        return;
    }

    const FVector CameraBefore = Pawn->Camera->GetComponentLocation();
    FRotator Rotation = Pawn->GetActorRotation();
    Rotation.Yaw += DeltaYawDegrees;
    Pawn->SetActorRotation(Rotation);

    const FVector CameraAfter = Pawn->Camera->GetComponentLocation();
    const FVector Correction = CameraBefore - CameraAfter;
    Pawn->AddActorWorldOffset(FVector(Correction.X, Correction.Y, 0.0f), false);
}

void UInterVerseVRLocomotionComponent::SnapTurn(float Direction)
{
    if (FMath::Abs(Direction) < 0.5f)
    {
        return;
    }

    RotatePawnAroundCamera(FMath::Sign(Direction) * SnapTurnDegrees);
}

void UInterVerseVRLocomotionComponent::BeginTeleportAim()
{
    if (!bTeleportEnabled)
    {
        return;
    }

    bTeleportAiming = true;
    UpdateTeleportAim();
}

void UInterVerseVRLocomotionComponent::GetTeleportArcPoints(TArray<FVector>& OutPoints, bool& bOutValidDestination) const
{
    OutPoints.Reset();
    bOutValidDestination = false;

    const AInterVerseXRPawn* Pawn = GetXRPawn();
    const UWorld* World = GetWorld();
    if (!Pawn || !Pawn->RightController || !World)
    {
        return;
    }

    FVector Position = Pawn->RightController->GetComponentLocation();
    FVector Velocity = Pawn->RightController->GetForwardVector() * TeleportLaunchSpeedCmPerSecond;
    const FVector Gravity(0.0f, 0.0f, World->GetGravityZ());
    const float StepSeconds = TeleportSimulationSeconds / FMath::Max(1, TeleportSimulationSteps);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(InterVerseTeleportArcVisual), false, Pawn);

    OutPoints.Add(Position);
    for (int32 Step = 0; Step < TeleportSimulationSteps; ++Step)
    {
        const FVector NextPosition = Position + Velocity * StepSeconds + 0.5f * Gravity * StepSeconds * StepSeconds;
        FHitResult Hit;
        if (World->LineTraceSingleByChannel(Hit, Position, NextPosition, ECC_Visibility, Params))
        {
            OutPoints.Add(Hit.ImpactPoint);
            bOutValidDestination = Hit.ImpactNormal.Z >= 0.65f;
            return;
        }

        OutPoints.Add(NextPosition);
        Position = NextPosition;
        Velocity += Gravity * StepSeconds;
    }
}

bool UInterVerseVRLocomotionComponent::FindTeleportDestination(FVector& OutDestination) const
{
    const AInterVerseXRPawn* Pawn = GetXRPawn();
    const UWorld* World = GetWorld();
    if (!Pawn || !Pawn->RightController || !World)
    {
        return false;
    }

    FVector Position = Pawn->RightController->GetComponentLocation();
    FVector Velocity = Pawn->RightController->GetForwardVector() * TeleportLaunchSpeedCmPerSecond;
    const FVector Gravity(0.0f, 0.0f, World->GetGravityZ());
    const float StepSeconds = TeleportSimulationSeconds / FMath::Max(1, TeleportSimulationSteps);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(InterVerseTeleportArc), false, Pawn);

    for (int32 Step = 0; Step < TeleportSimulationSteps; ++Step)
    {
        const FVector NextPosition = Position + Velocity * StepSeconds + 0.5f * Gravity * StepSeconds * StepSeconds;
        FHitResult Hit;
        if (World->LineTraceSingleByChannel(Hit, Position, NextPosition, ECC_Visibility, Params))
        {
            if (Hit.ImpactNormal.Z >= 0.65f)
            {
                OutDestination = Hit.ImpactPoint + FVector(0.0f, 0.0f, TeleportDestinationLiftCm);
                return true;
            }
            return false;
        }

        Position = NextPosition;
        Velocity += Gravity * StepSeconds;
    }

    return false;
}

bool UInterVerseVRLocomotionComponent::UpdateTeleportAim()
{
    if (!bTeleportAiming || !bTeleportEnabled)
    {
        return false;
    }

    FVector Candidate;
    bHasValidTeleportDestination = FindTeleportDestination(Candidate);
    if (bHasValidTeleportDestination)
    {
        TeleportDestination = Candidate;
    }

    OnTeleportAimChanged.Broadcast(bHasValidTeleportDestination, TeleportDestination);
    return bHasValidTeleportDestination;
}

bool UInterVerseVRLocomotionComponent::CommitTeleport()
{
    if (!bTeleportAiming)
    {
        return false;
    }

    UpdateTeleportAim();
    AInterVerseXRPawn* Pawn = GetXRPawn();
    if (!Pawn || !Pawn->Camera || !bHasValidTeleportDestination)
    {
        CancelTeleport();
        return false;
    }

    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector CameraLocation = Pawn->Camera->GetComponentLocation();
    const FVector CameraOffsetXY(CameraLocation.X - PawnLocation.X, CameraLocation.Y - PawnLocation.Y, 0.0f);
    const FVector TargetPawnLocation = TeleportDestination - CameraOffsetXY;

    const bool bMoved = Pawn->TeleportTo(TargetPawnLocation, Pawn->GetActorRotation(), false, true);
    const FVector CompletedDestination = TeleportDestination;
    CancelTeleport();

    if (bMoved)
    {
        OnTeleportCompleted.Broadcast(CompletedDestination);
    }
    return bMoved;
}

void UInterVerseVRLocomotionComponent::CancelTeleport()
{
    bTeleportAiming = false;
    bHasValidTeleportDestination = false;
    TeleportDestination = FVector::ZeroVector;
    OnTeleportAimChanged.Broadcast(false, TeleportDestination);
}
