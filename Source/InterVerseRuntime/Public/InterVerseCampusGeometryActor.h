#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseCampusGeometryActor.generated.h"

class USceneComponent;

/**
 * Editor/runtime visualization of verified InterVerseSG campus geometry.
 *
 * The actor reads Config/InterVerseCampusGeometry.local.json and creates
 * lightweight spline outlines for Polygon/LineString features. This keeps the
 * geospatial source separate from the final optimized Quest meshes.
 */
UCLASS(BlueprintType)
class AInterVerseCampusGeometryActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseCampusGeometryActor();

    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry")
    FString GeometryConfigRelativePath = TEXT("InterVerseCampusGeometry.local.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry")
    float ZOffsetCm = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry")
    bool bClosedBuildingLoops = true;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="InterVerse|Geometry")
    void RebuildGeometry();

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    void ClearGeneratedSplines();
    void AddSplineFromPoints(const TArray<FVector>& Points, bool bClosedLoop, const FString& FeatureId);
};
