#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseBuildingExtrusionActor.generated.h"

class UMaterialInterface;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry")
    FString SectorJsonRelativePath = TEXT("Config/InterVerseCampusSectors.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry", meta=(ClampMin="100.0"))
    float DefaultBuildingHeightCm = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Geometry", meta=(ClampMin="100.0"))
    float FloorHeightCm = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance")
    bool bCreateCollision = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance")
    bool bEnableRuntimeSectorCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="10000.0"))
    float ActiveSectorRadiusCm = 36000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="0.1"))
    float SectorUpdateIntervalSeconds = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Materials")
    TObjectPtr<UMaterialInterface> NearBuildingMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Materials")
    TObjectPtr<UMaterialInterface> FarBuildingMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Materials", meta=(ClampMin="5000.0"))
    float NearMaterialDistanceCm = 18000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Materials", meta=(ClampMin="50.0"))
    float FacadeURepeatCm = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Quest Materials", meta=(ClampMin="50.0"))
    float FacadeVRepeatCm = 300.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    int32 LastBuiltPolygonCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    int32 LastMeshSectionCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    TArray<FString> BuiltSectorIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    TArray<FVector2D> BuiltSectorCentersCm;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse")
    bool RebuildBuildings();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse")
    void ClearBuildings();

    UFUNCTION(BlueprintCallable, Category="InterVerse|Performance")
    void UpdateSectorVisibility();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

private:
    FTimerHandle SectorVisibilityTimer;
    TArray<bool> SectorUsesNearMaterial;

    bool ParseAndBuild(const FString& JsonText);
    bool LoadSectorDefinitions(TArray<FString>& OutIds, TArray<FVector2D>& OutCenters) const;
    int32 FindNearestSector(const FVector2D& Point, const TArray<FVector2D>& Centers) const;
    float ResolveHeightCm(const TSharedPtr<class FJsonObject>& Properties) const;
    float ResolveBaseZCm(const TSharedPtr<class FJsonObject>& Properties) const;

    bool AppendPolygonGeometry(
        const TArray<FVector2D>& Polygon,
        float BaseZCm,
        float HeightCm,
        TArray<FVector>& Vertices,
        TArray<int32>& Triangles,
        TArray<FVector2D>& UV0,
        TArray<FLinearColor>& VertexColors) const;

    static bool TriangulatePolygon(
        const TArray<FVector2D>& Polygon,
        TArray<int32>& OutTriangles);
};
