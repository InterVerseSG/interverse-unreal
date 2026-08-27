#include "InterVerseXRPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "InterVerseCloudClient.h"
#include "InterVerseNavigationComponent.h"
#include "InterVerseVRLocomotionComponent.h"
#include "MotionControllerComponent.h"

AInterVerseXRPawn::AInterVerseXRPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
    SetRootComponent(VROrigin);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(VROrigin);
    Camera->bLockToHmd = true;

    LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
    LeftController->SetupAttachment(VROrigin);
    LeftController->SetTrackingMotionSource(FName(TEXT("Left")));

    RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
    RightController->SetupAttachment(VROrigin);
    RightController->SetTrackingMotionSource(FName(TEXT("Right")));

    CloudClient = CreateDefaultSubobject<UInterVerseCloudClient>(TEXT("InterVerseCloudClient"));
    Navigation = CreateDefaultSubobject<UInterVerseNavigationComponent>(TEXT("InterVerseNavigation"));
    Locomotion = CreateDefaultSubobject<UInterVerseVRLocomotionComponent>(TEXT("InterVerseVRLocomotion"));
}

void AInterVerseXRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("IV_MoveForward"), this, &AInterVerseXRPawn::InputMoveForward);
    PlayerInputComponent->BindAxis(TEXT("IV_MoveRight"), this, &AInterVerseXRPawn::InputMoveRight);
    PlayerInputComponent->BindAxis(TEXT("IV_Turn"), this, &AInterVerseXRPawn::InputTurn);
    PlayerInputComponent->BindAction(TEXT("IV_Teleport"), IE_Pressed, this, &AInterVerseXRPawn::InputTeleportPressed);
    PlayerInputComponent->BindAction(TEXT("IV_Teleport"), IE_Released, this, &AInterVerseXRPawn::InputTeleportReleased);
}

void AInterVerseXRPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (Locomotion)
    {
        Locomotion->SmoothMove(FVector2D(MoveRightValue, MoveForwardValue), DeltaSeconds);
        if (Locomotion->IsTeleportAiming())
        {
            Locomotion->UpdateTeleportAim();
        }
    }
}

void AInterVerseXRPawn::InputMoveForward(float Value)
{
    MoveForwardValue = Value;
}

void AInterVerseXRPawn::InputMoveRight(float Value)
{
    MoveRightValue = Value;
}

void AInterVerseXRPawn::InputTurn(float Value)
{
    if (!Locomotion)
    {
        return;
    }

    if (FMath::Abs(Value) >= 0.7f)
    {
        if (!bTurnLatched)
        {
            Locomotion->SnapTurn(Value);
            bTurnLatched = true;
        }
    }
    else if (FMath::Abs(Value) <= 0.25f)
    {
        bTurnLatched = false;
    }
}

void AInterVerseXRPawn::InputTeleportPressed()
{
    if (Locomotion)
    {
        Locomotion->BeginTeleportAim();
    }
}

void AInterVerseXRPawn::InputTeleportReleased()
{
    if (Locomotion)
    {
        Locomotion->CommitTeleport();
    }
}
