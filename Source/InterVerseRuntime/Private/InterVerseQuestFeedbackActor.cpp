#include "InterVerseQuestFeedbackActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InterVerseNavigationComponent.h"
#include "InterVerseVRLocomotionComponent.h"
#include "InterVerseXRPawn.h"
#include "Kismet/GameplayStatics.h"

AInterVerseQuestFeedbackActor::AInterVerseQuestFeedbackActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.016f;
}

void AInterVerseQuestFeedbackActor::BeginPlay()
{
    Super::BeginPlay();
    ResolvePawn();
}

void AInterVerseQuestFeedbackActor::ResolvePawn()
{
    if (XRPawn) return;

    XRPawn = Cast<AInterVerseXRPawn>(UGameplayStatics::GetActorOfClass(GetWorld(), AInterVerseXRPawn::StaticClass()));
    if (XRPawn && XRPawn->Locomotion)
    {
        XRPawn->Locomotion->OnTeleportCompleted.AddDynamic(this, &AInterVerseQuestFeedbackActor::HandleTeleportCompleted);
    }
}

void AInterVerseQuestFeedbackActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ResolvePawn();
    if (!XRPawn) return;

    if (RightHapticRemaining > 0.0f)
    {
        RightHapticRemaining -= DeltaSeconds;
        if (RightHapticRemaining <= 0.0f) StopRightHaptics();
    }

    UpdatePointerFeedback();
    UpdateArrivalFeedback();
    UpdateRightControllerScale();
}

void AInterVerseQuestFeedbackActor::PulseRight(float Amplitude, float DurationSeconds)
{
    if (!bEnableHaptics) return;

    APlayerController* PC = Cast<APlayerController>(XRPawn ? XRPawn->GetController() : nullptr);
    if (!PC) PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC) return;

    PC->SetHapticsByValue(1.0f, FMath::Clamp(Amplitude, 0.0f, 1.0f), EControllerHand::Right);
    RightHapticRemaining = FMath::Max(DurationSeconds, 0.01f);
}

void AInterVerseQuestFeedbackActor::StopRightHaptics()
{
    APlayerController* PC = Cast<APlayerController>(XRPawn ? XRPawn->GetController() : nullptr);
    if (!PC) PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC) PC->SetHapticsByValue(0.0f, 0.0f, EControllerHand::Right);
    RightHapticRemaining = 0.0f;
}

void AInterVerseQuestFeedbackActor::UpdatePointerFeedback()
{
    const bool bMenuVisible = XRPawn->IsVRMenuVisible();
    const bool bHover = bMenuVisible && XRPawn->IsPointerHoveringWidget();
    const bool bPressed = bMenuVisible && XRPawn->IsPointerPressed();

    if (bHover && !bLastHover && !bPressed)
    {
        PulseRight(HoverAmplitude, ShortPulseSeconds);
    }
    if (bPressed && !bLastPressed)
    {
        PulseRight(PressAmplitude, ConfirmPulseSeconds);
    }

    bLastHover = bHover;
    bLastPressed = bPressed;
}

void AInterVerseQuestFeedbackActor::UpdateArrivalFeedback()
{
    if (!XRPawn->Navigation || !XRPawn->Navigation->IsGuidanceActive())
    {
        bArrivalLatched = false;
        return;
    }

    const float Distance = XRPawn->Navigation->GetGuidanceDistanceCm();
    if (Distance <= ArrivalDistanceCm)
    {
        if (!bArrivalLatched)
        {
            bArrivalLatched = true;
            PulseRight(ArrivalAmplitude, ConfirmPulseSeconds * 1.35f);
        }
    }
    else if (Distance > ArrivalDistanceCm * 1.35f)
    {
        bArrivalLatched = false;
    }
}

void AInterVerseQuestFeedbackActor::UpdateRightControllerScale()
{
    if (!XRPawn->RightControllerVisual) return;

    const FVector BaseScale(0.10f, 0.045f, 0.035f);
    if (!bEnableControllerScaleFeedback || !XRPawn->IsVRMenuVisible())
    {
        XRPawn->RightControllerVisual->SetRelativeScale3D(BaseScale);
        return;
    }

    if (XRPawn->IsPointerPressed())
    {
        XRPawn->RightControllerVisual->SetRelativeScale3D(BaseScale * 0.94f);
    }
    else if (XRPawn->IsPointerHoveringWidget())
    {
        XRPawn->RightControllerVisual->SetRelativeScale3D(BaseScale * 1.08f);
    }
    else
    {
        XRPawn->RightControllerVisual->SetRelativeScale3D(BaseScale);
    }
}

void AInterVerseQuestFeedbackActor::HandleTeleportCompleted(FVector Destination)
{
    PulseRight(TeleportAmplitude, ConfirmPulseSeconds);
}
