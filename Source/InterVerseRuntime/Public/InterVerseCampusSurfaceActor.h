#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseCampusSurfaceActor.generated.h"

class UProceduralMeshComponent;
class FJsonObject;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseCampusSurfaceActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseCampusSurfaceActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Campus")
    TObjectPtr<UProceduralMeshComponent> SurfaceMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Campus")
    FString SurfacesJsonRelativePath = TEXT("Config/InterVerseCampusSurfaces.local.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Campus", meta=(ClampMin="50.0"))
    float RoadWidthCm = 550.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Campus", meta=(ClampMin="25.0"))
    float PedestrianWidthCm = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Campus", meta=(ClampMin="25.0"))
    float ParkingOutlineWidthCm = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Campus")
    float SurfaceZCm = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Campus")
    bool bCreateCollision = false;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="InterVerse|Campus")
    bool RebuildSurfaces();

    UFUNCTION(CallInEditor, BlueprintCallable, Category="InterVerse|Campus")
    void ClearSurfaces();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    bool ParseAndBuild(const FString& JsonText);
    float WidthForCategory(const FString& Category) const;
    void AddPolylineRibbon(
        const TArray<FVector2D>& Points,
        float WidthCm,
        TArray<FVector>& Vertices,
        TArray<int32>& Triangles) const;
};
