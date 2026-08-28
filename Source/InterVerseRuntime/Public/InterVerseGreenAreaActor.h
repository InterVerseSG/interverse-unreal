#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseGreenAreaActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseGreenAreaActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseGreenAreaActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|GreenAreas")
    TObjectPtr<UProceduralMeshComponent> GreenMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|GreenAreas")
    FString GreenAreaJsonRelativePath = TEXT("Config/InterVerseGreenAreas.local.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|GreenAreas")
    TObjectPtr<UMaterialInterface> GreenAreaMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|GreenAreas")
    float SurfaceOffsetCm = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance")
    bool bCreateCollision = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    int32 LastPolygonCount = 0;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse|GreenAreas")
    bool RebuildGreenAreas();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse|GreenAreas")
    void ClearGreenAreas();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    bool ParseAndBuild(const FString& JsonText);
    static bool TriangulatePolygon(const TArray<FVector>& Polygon, TArray<int32>& OutTriangles);
};
