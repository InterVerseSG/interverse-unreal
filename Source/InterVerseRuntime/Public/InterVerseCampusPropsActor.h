#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterVerseCampusPropsActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

UCLASS(BlueprintType)
class INTERVERSERUNTIME_API AInterVerseCampusPropsActor : public AActor
{
    GENERATED_BODY()

public:
    AInterVerseCampusPropsActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Props")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Props")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BenchStandard;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Props")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BenchPriority;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Props")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LampStandard;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Props")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LampPriority;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Props")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SignStandard;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Props")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SignPriority;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Props")
    TObjectPtr<UStaticMesh> BenchMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Props")
    TObjectPtr<UStaticMesh> LampMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Props")
    TObjectPtr<UStaticMesh> SignMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Props")
    FString PropsJsonRelativePath = TEXT("Config/InterVerseCampusProps.local.json");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="1000"))
    int32 StandardStartCullDistanceCm = 7000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="1000"))
    int32 StandardEndCullDistanceCm = 18000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="1000"))
    int32 PriorityStartCullDistanceCm = 12000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance", meta=(ClampMin="1000"))
    int32 PriorityEndCullDistanceCm = 30000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InterVerse|Performance")
    bool bCastShadows = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    int32 LastBenchCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    int32 LastLampCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="InterVerse|Performance")
    int32 LastSignCount = 0;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse|Props")
    bool RebuildProps();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="InterVerse|Props")
    void ClearProps();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    void ConfigureHISM(UHierarchicalInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, bool bPriority) const;
};
