#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseTerrainActor.generated.h"

class UProceduralMeshComponent;

UCLASS()
class INTERVERSERUNTIME_API AInterVerseTerrainActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseTerrainActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Terrain")
    UProceduralMeshComponent* TerrainMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Terrain")
    FString TerrainJsonRelativePath = TEXT("Data/campus_terrain_grid.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Terrain")
    bool bCreateCollision = true;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="InterVerse|Terrain")
    bool RebuildTerrain();

    UFUNCTION(CallInEditor, BlueprintCallable, Category="InterVerse|Terrain")
    void ClearTerrain();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    bool ParseAndBuild(const FString& JsonText);
};
