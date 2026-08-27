#include "InterVerseXRPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InterVerseCloudClient.h"
#include "InterVerseNavigationComponent.h"
#include "InterVerseVRLocomotionComponent.h"
#include "MotionControllerComponent.h"
#include "ProceduralMeshComponent.h"

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

    TeleportVisualMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TeleportVisualMesh"));
    TeleportVisualMesh->SetupAttachment(VROrigin);
    TeleportVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TeleportVisualMesh->SetHiddenInGame(true);

    GuidanceArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("GuidanceArrow"));
    GuidanceArrow->SetupAttachment(Camera);
    GuidanceArrow->SetRelativeLocation(FVector(100.0f, 0.0f, -18.0f));
    GuidanceArrow->ArrowSize = 0.65f;
    GuidanceArrow->SetHiddenInGame(true);

    GuidanceText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("GuidanceText"));
    GuidanceText->SetupAttachment(Camera);
    GuidanceText->SetRelativeLocation(FVector(105.0f, 0.0f, -32.0f));
    GuidanceText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    GuidanceText->SetHorizontalAlignment(EHTA_Center);
    GuidanceText->SetWorldSize(7.0f);
    GuidanceText->SetHiddenInGame(true);
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
            UpdateTeleportVisual();
        }
        else
        {
            ClearTeleportVisual();
        }
    }

    if (Navigation && Navigation->IsGuidanceActive())
    {
        Navigation->UpdateGuidance();
        UpdateGuidanceVisual();
    }
    else
    {
        if (GuidanceArrow) GuidanceArrow->SetHiddenInGame(true);
        if (GuidanceText) GuidanceText->SetHiddenInGame(true);
    }
}

void AInterVerseXRPawn::UpdateTeleportVisual()
{
    if (!TeleportVisualMesh || !Locomotion || !Locomotion->IsTeleportAiming())
    {
        ClearTeleportVisual();
        return;
    }

    TArray<FVector> ArcWorld;
    bool bValidDestination = false;
    Locomotion->GetTeleportArcPoints(ArcWorld, bValidDestination);
    if (ArcWorld.Num() < 2)
    {
        ClearTeleportVisual();
        return;
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    const FTransform RootTransform = VROrigin->GetComponentTransform();
    const float HalfWidth = TeleportArcWidthCm * 0.5f;

    for (int32 Index = 0; Index < ArcWorld.Num() - 1; ++Index)
    {
        const FVector A = ArcWorld[Index];
        const FVector B = ArcWorld[Index + 1];
        FVector Direction = B - A;
        if (!Direction.Normalize())
        {
            continue;
        }
        FVector Side = FVector::CrossProduct(Direction, FVector::UpVector);
        if (!Side.Normalize())
        {
            Side = FVector::RightVector;
        }
        Side *= HalfWidth;

        const int32 Base = Vertices.Num();
        Vertices.Add(RootTransform.InverseTransformPosition(A + Side));
        Vertices.Add(RootTransform.InverseTransformPosition(A - Side));
        Vertices.Add(RootTransform.InverseTransformPosition(B + Side));
        Vertices.Add(RootTransform.InverseTransformPosition(B - Side));
        Triangles.Append({Base, Base + 2, Base + 1, Base + 1, Base + 2, Base + 3});
    }

    if (bValidDestination)
    {
        const FVector CenterWorld = ArcWorld.Last() + FVector(0.0f, 0.0f, 1.0f);
        const FVector Center = RootTransform.InverseTransformPosition(CenterWorld);
        const int32 Segments = 20;
        const int32 CenterIndex = Vertices.Num();
        Vertices.Add(Center);
        for (int32 I = 0; I <= Segments; ++I)
        {
            const float Angle = 2.0f * PI * static_cast<float>(I) / static_cast<float>(Segments);
            const FVector PWorld = CenterWorld + FVector(FMath::Cos(Angle) * TeleportMarkerRadiusCm, FMath::Sin(Angle) * TeleportMarkerRadiusCm, 0.0f);
            Vertices.Add(RootTransform.InverseTransformPosition(PWorld));
        }
        for (int32 I = 0; I < Segments; ++I)
        {
            Triangles.Append({CenterIndex, CenterIndex + I + 1, CenterIndex + I + 2});
        }
    }

    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    TeleportVisualMesh->ClearAllMeshSections();
    TeleportVisualMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);
    TeleportVisualMesh->SetHiddenInGame(false);
}

void AInterVerseXRPawn::ClearTeleportVisual()
{
    if (TeleportVisualMesh && !TeleportVisualMesh->bHiddenInGame)
    {
        TeleportVisualMesh->ClearAllMeshSections();
        TeleportVisualMesh->SetHiddenInGame(true);
    }
}

void AInterVerseXRPawn::UpdateGuidanceVisual()
{
    if (!Navigation || !Navigation->IsGuidanceActive() || !Camera || !GuidanceArrow || !GuidanceText)
    {
        return;
    }

    FVector Direction = Navigation->GetGuidanceDirection();
    Direction.Z = 0.0f;
    if (!Direction.Normalize())
    {
        return;
    }

    const float TargetYaw = Direction.Rotation().Yaw;
    const float CameraYaw = Camera->GetComponentRotation().Yaw;
    const float RelativeYaw = FMath::FindDeltaAngleDegrees(CameraYaw, TargetYaw);
    GuidanceArrow->SetRelativeRotation(FRotator(0.0f, RelativeYaw, 0.0f));
    GuidanceArrow->SetHiddenInGame(false);

    const float DistanceMeters = Navigation->GetGuidanceDistanceCm() / 100.0f;
    FString Name = Navigation->GetGuidanceAnchor();
    Name.RemoveFromStart(TEXT("NAV_"));
    GuidanceText->SetText(FText::FromString(FString::Printf(TEXT("%s  %.0f m"), *Name, DistanceMeters)));
    GuidanceText->SetHiddenInGame(false);
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
        ClearTeleportVisual();
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
