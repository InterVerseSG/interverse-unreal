#include "InterVerseXRPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
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

void AInterVerseXRPawn::BeginPlay()
{
    Super::BeginPlay();
    EnsureEnhancedInputMappings();
}

void AInterVerseXRPawn::EnsureEnhancedInputMappings()
{
    if (!bEnableRuntimeQuestMappings)
    {
        return;
    }

    if (!RuntimeMappingContext)
    {
        RuntimeMappingContext = NewObject<UInputMappingContext>(this, TEXT("IV_RuntimeQuestMapping"));

        MoveForwardAction = NewObject<UInputAction>(this, TEXT("IV_IA_MoveForward"));
        MoveForwardAction->ValueType = EInputActionValueType::Axis1D;
        MoveRightAction = NewObject<UInputAction>(this, TEXT("IV_IA_MoveRight"));
        MoveRightAction->ValueType = EInputActionValueType::Axis1D;
        TurnAction = NewObject<UInputAction>(this, TEXT("IV_IA_Turn"));
        TurnAction->ValueType = EInputActionValueType::Axis1D;
        TeleportAction = NewObject<UInputAction>(this, TEXT("IV_IA_Teleport"));
        TeleportAction->ValueType = EInputActionValueType::Boolean;

        // Meta/Oculus Touch keys exposed by Unreal's OpenXR/Oculus controller profiles.
        RuntimeMappingContext->MapKey(MoveForwardAction, FKey(FName(TEXT("OculusTouch_Left_Thumbstick_Y"))));
        RuntimeMappingContext->MapKey(MoveRightAction, FKey(FName(TEXT("OculusTouch_Left_Thumbstick_X"))));
        RuntimeMappingContext->MapKey(TurnAction, FKey(FName(TEXT("OculusTouch_Right_Thumbstick_X"))));
        RuntimeMappingContext->MapKey(TeleportAction, FKey(FName(TEXT("OculusTouch_Right_Trigger_Click"))));
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->GetLocalPlayer() || !RuntimeMappingContext)
    {
        return;
    }

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
    {
        Subsystem->AddMappingContext(RuntimeMappingContext, 10);
    }
}

void AInterVerseXRPawn::BindEnhancedInput(UInputComponent* PlayerInputComponent)
{
    UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!Enhanced || !MoveForwardAction || !MoveRightAction || !TurnAction || !TeleportAction)
    {
        return;
    }

    Enhanced->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &AInterVerseXRPawn::EnhancedMoveForward);
    Enhanced->BindAction(MoveForwardAction, ETriggerEvent::Completed, this, &AInterVerseXRPawn::EnhancedMoveForward);
    Enhanced->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &AInterVerseXRPawn::EnhancedMoveRight);
    Enhanced->BindAction(MoveRightAction, ETriggerEvent::Completed, this, &AInterVerseXRPawn::EnhancedMoveRight);
    Enhanced->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AInterVerseXRPawn::EnhancedTurn);
    Enhanced->BindAction(TurnAction, ETriggerEvent::Completed, this, &AInterVerseXRPawn::EnhancedTurn);
    Enhanced->BindAction(TeleportAction, ETriggerEvent::Started, this, &AInterVerseXRPawn::EnhancedTeleportStarted);
    Enhanced->BindAction(TeleportAction, ETriggerEvent::Completed, this, &AInterVerseXRPawn::EnhancedTeleportCompleted);
}

void AInterVerseXRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    EnsureEnhancedInputMappings();
    BindEnhancedInput(PlayerInputComponent);

    // Legacy fallback remains useful for desktop keyboard/gamepad testing.
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

    if (Navigation && Navigation->IsGuidanceActive())
    {
        Navigation->UpdateGuidance();
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

void AInterVerseXRPawn::EnhancedMoveForward(const FInputActionValue& Value)
{
    InputMoveForward(Value.Get<float>());
}

void AInterVerseXRPawn::EnhancedMoveRight(const FInputActionValue& Value)
{
    InputMoveRight(Value.Get<float>());
}

void AInterVerseXRPawn::EnhancedTurn(const FInputActionValue& Value)
{
    InputTurn(Value.Get<float>());
}

void AInterVerseXRPawn::EnhancedTeleportStarted(const FInputActionValue& Value)
{
    if (Value.Get<bool>())
    {
        InputTeleportPressed();
    }
}

void AInterVerseXRPawn::EnhancedTeleportCompleted(const FInputActionValue& Value)
{
    InputTeleportReleased();
}
