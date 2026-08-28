#include "InterVerseVRHudActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "InterVerseVRHudWidget.h"
#include "InterVerseXRPawn.h"

AInterVerseVRHudActor::AInterVerseVRHudActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    HudWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NavigationHUD"));
    HudWidget->SetupAttachment(Root);
    HudWidget->SetWidgetSpace(EWidgetSpace::World);
    HudWidget->SetWidgetClass(UInterVerseVRHudWidget::StaticClass());
    HudWidget->SetDrawSize(DrawSize);
    HudWidget->SetPivot(FVector2D(0.5f, 0.5f));
    HudWidget->SetTwoSided(true);
    HudWidget->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    HudWidget->SetRelativeScale3D(FVector(0.10f));
    HudWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AInterVerseVRHudActor::BeginPlay()
{
    Super::BeginPlay();
    AttachToLocalPlayerCamera();
}

void AInterVerseVRHudActor::AttachToLocalPlayerCamera()
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    AInterVerseXRPawn* Pawn = PC ? Cast<AInterVerseXRPawn>(PC->GetPawn()) : nullptr;
    if (!Pawn || !Pawn->Camera)
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: VR HUD could not find InterVerseXRPawn camera."));
        return;
    }

    AttachToComponent(Pawn->Camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    SetActorRelativeLocation(RelativeLocationCm);
    SetActorRelativeRotation(FRotator::ZeroRotator);

    if (HudWidget)
    {
        HudWidget->SetOwnerPlayer(PC->GetLocalPlayer());
        HudWidget->SetDrawSize(DrawSize);
    }

    UE_LOG(LogTemp, Log, TEXT("InterVerseSG: Quest navigation HUD attached to player camera."));
}
