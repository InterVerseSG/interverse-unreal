#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseBuildingExtrusionActor.generated.h"

class UProceduralMeshComponent;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseBuildingExtrusionActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseBuildingExtrusionActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse")
    TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry")
    FString GeometryJsonRelativePath = TEXT("Config/InterVerseCampusGeometry.local.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry", meta=(ClampMin="100.0"))
    float DefaultBuildingHeightCm = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry", meta=(ClampMin="100.0"))
    float FloorHeightCm = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance")
    bool bCreateCollision = false;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse")
    bool RebuildBuildings();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse")
    void ClearBuildings();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    bool ParseAndBuild(const FString& JsonText);
    float ResolveHeightCm(const TSharedPtr<class FJsonObject>& Properties) const;
    bool BuildPolygonSection(
        const TArray<FVector2D>& Polygon,
        float HeightCm,
        int32 SectionIndex
    );

    static bool TriangulatePolygon(
        const TArray<FVector2D>& Polygon,
        TArray<int32>& OutTriangles
    );
};
