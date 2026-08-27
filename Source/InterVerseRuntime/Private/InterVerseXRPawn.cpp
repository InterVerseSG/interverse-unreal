#include "InterVerseXRPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "InterVerseCloudClient.h"
#include "InterVerseNavigationComponent.h"
#include "MotionControllerComponent.h"

AInterVerseXRPawn::AInterVerseXRPawn()
{
    PrimaryActorTick.bCanEverTick = false;
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
}
