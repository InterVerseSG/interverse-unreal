#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "InterVerseXRPawn.generated.h"

class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UInterVerseCloudClient;
class UInterVerseNavigationComponent;
class UInterVerseVRLocomotionComponent;
class UInputAction;
class UInputMappingContext;
class UProceduralMeshComponent;
class UArrowComponent;
class UTextRenderComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseXRPawn : public APawn
{
    GENERATED_BODY()

public:
    AInterVerseXRPawn();

    virtual void BeginPlay() override;
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
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|Visuals")
    TObjectPtr<UProceduralMeshComponent> TeleportVisualMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|Visuals")
    TObjectPtr<UArrowComponent> GuidanceArrow;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|Visuals")
    TObjectPtr<UTextRenderComponent> GuidanceText;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|Menu")
    TObjectPtr<UWidgetComponent> VRMenuWidget;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|Menu")
    TObjectPtr<UWidgetInteractionComponent> RightWidgetInteraction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|Interaction")
    TObjectPtr<UProceduralMeshComponent> RightPointerVisual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|XR|Input")
    bool bEnableRuntimeQuestMappings = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Visuals")
    float TeleportArcWidthCm = 3.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Visuals")
    float TeleportMarkerRadiusCm = 22.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Interaction", meta=(ClampMin="0.1"))
    float PointerWidthCm = 0.7f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|Interaction", meta=(ClampMin="100.0"))
    float PointerMaxDistanceCm = 500.0f;

    UFUNCTION(BlueprintCallable, Category="InterVerse|VR|Menu")
    void SetVRMenuVisible(bool bVisible);
    UFUNCTION(BlueprintPure, Category="InterVerse|VR|Menu")
    bool IsVRMenuVisible() const;
    UFUNCTION(BlueprintPure, Category="InterVerse|VR|Interaction")
    bool IsPointerHoveringWidget() const;

private:
    void EnsureEnhancedInputMappings();
    void BindEnhancedInput(UInputComponent* PlayerInputComponent);
    void UpdateTeleportVisual();
    void ClearTeleportVisual();
    void UpdateGuidanceVisual();
    void UpdatePointerVisual();
    void ClearPointerVisual();
    void ToggleVRMenu();
    void InputMoveForward(float Value);
    void InputMoveRight(float Value);
    void InputTurn(float Value);
    void InputTeleportPressed();
    void InputTeleportReleased();
    void EnhancedMoveForward(const FInputActionValue& Value);
    void EnhancedMoveRight(const FInputActionValue& Value);
    void EnhancedTurn(const FInputActionValue& Value);
    void EnhancedTeleportStarted(const FInputActionValue& Value);
    void EnhancedTeleportCompleted(const FInputActionValue& Value);
    void EnhancedMenuStarted(const FInputActionValue& Value);

    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> RuntimeMappingContext;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MoveForwardAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MoveRightAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> TurnAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> TeleportAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MenuAction;

    float MoveForwardValue = 0.0f;
    float MoveRightValue = 0.0f;
    bool bTurnLatched = false;
};
