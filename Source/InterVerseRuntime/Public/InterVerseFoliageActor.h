#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseFoliageActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseFoliageActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseFoliageActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Foliage")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Foliage")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeInstances;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Foliage")
    TObjectPtr<UStaticMesh> TreeMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Foliage")
    FString FoliageJsonRelativePath = TEXT("Config/InterVerseFoliage.local.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="0"))
    int32 StartCullDistanceCm = 12000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="1000"))
    int32 EndCullDistanceCm = 30000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance")
    bool bCastShadows = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    int32 LastInstanceCount = 0;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse|Foliage")
    bool RebuildFoliage();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse|Foliage")
    void ClearFoliage();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
};
