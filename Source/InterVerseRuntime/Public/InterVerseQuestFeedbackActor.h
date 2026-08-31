#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseQuestFeedbackActor.generated.h"

class AInterVerseXRPawn;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseQuestFeedbackActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseQuestFeedbackActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback")
    bool bEnableHaptics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback")
    bool bEnableControllerScaleFeedback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback|Haptics", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HoverAmplitude = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback|Haptics", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PressAmplitude = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback|Haptics", meta=(ClampMin="0.0", ClampMax="1.0"))
    float TeleportAmplitude = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback|Haptics", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ArrivalAmplitude = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback|Haptics", meta=(ClampMin="0.01", ClampMax="0.5"))
    float ShortPulseSeconds = 0.045f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback|Haptics", meta=(ClampMin="0.01", ClampMax="0.5"))
    float ConfirmPulseSeconds = 0.09f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Feedback|Navigation", meta=(ClampMin="100.0"))
    float ArrivalDistanceCm = 1200.0f;

private:
    UFUNCTION()
    void HandleTeleportCompleted(FVector Destination);

    void ResolvePawn();
    void PulseRight(float Amplitude, float DurationSeconds);
    void StopRightHaptics();
    void UpdatePointerFeedback();
    void UpdateArrivalFeedback();
    void UpdateRightControllerScale();

    UPROPERTY(Transient)
    TObjectPtr<AInterVerseXRPawn> XRPawn;

    bool bLastHover = false;
    bool bLastPressed = false;
    bool bArrivalLatched = false;
    float RightHapticRemaining = 0.0f;
};
