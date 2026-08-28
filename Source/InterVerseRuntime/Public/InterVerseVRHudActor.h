#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseVRHudActor.generated.h"

class UWidgetComponent;
class USceneComponent;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseVRHudActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseVRHudActor();
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|HUD")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|VR|HUD")
    TObjectPtr<UWidgetComponent> HudWidget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|HUD")
    FVector RelativeLocationCm = FVector(105.0f, 0.0f, -35.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|VR|HUD")
    FVector2D DrawSize = FVector2D(760.0f, 250.0f);

private:
    void AttachToLocalPlayerCamera();
};