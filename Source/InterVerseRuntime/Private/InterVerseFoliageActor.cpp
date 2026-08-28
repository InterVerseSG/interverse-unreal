#include "InterVerseFoliageActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AInterVerseFoliageActor::AInterVerseFoliageActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    TreeInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeInstances"));
    TreeInstances->SetupAttachment(SceneRoot);
    TreeInstances->SetMobility(EComponentMobility::Static);
    TreeInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TreeInstances->bCastDynamicShadow = false;
    TreeInstances->CastShadow = false;
}

void AInterVerseFoliageActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
#if WITH_EDITOR
    RebuildFoliage();
#endif
}

void AInterVerseFoliageActor::ClearFoliage()
{
    LastInstanceCount = 0;
    if (TreeInstances)
    {
        TreeInstances->ClearInstances();
    }
}

bool AInterVerseFoliageActor::RebuildFoliage()
{
    ClearFoliage();

    if (!TreeInstances || !TreeMesh)
    {
        UE_LOG(LogTemp, Log, TEXT("InterVerseSG: foliage HISM ready; assign TreeMesh to render instances."));
        return false;
    }

    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), FoliageJsonRelativePath));

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: foliage file not found: %s"), *AbsolutePath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: invalid foliage JSON."));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Instances = nullptr;
    if (!Root->TryGetArrayField(TEXT("instances"), Instances) || !Instances)
    {
        return false;
    }

    TreeInstances->SetStaticMesh(TreeMesh);
    TreeInstances->SetCullDistances(StartCullDistanceCm, EndCullDistanceCm);
    TreeInstances->CastShadow = bCastShadows;
    TreeInstances->bCastDynamicShadow = bCastShadows;

    for (const TSharedPtr<FJsonValue>& Value : *Instances)
    {
        const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid())
        {
            continue;
        }

        const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
        double X = 0.0, Y = 0.0, Z = 0.0, Yaw = 0.0, Scale = 1.0;
        if (!Obj->TryGetNumberField(TEXT("x_cm"), X) || !Obj->TryGetNumberField(TEXT("y_cm"), Y))
        {
            continue;
        }
        Obj->TryGetNumberField(TEXT("z_cm"), Z);
        Obj->TryGetNumberField(TEXT("yaw_deg"), Yaw);
        Obj->TryGetNumberField(TEXT("scale"), Scale);
        Scale = FMath::Clamp(Scale, 0.6, 1.4);

        const FTransform InstanceTransform(
            FRotator(0.0, Yaw, 0.0),
            FVector(X, Y, Z),
            FVector(Scale));
        TreeInstances->AddInstance(InstanceTransform, false);
    }

    LastInstanceCount = TreeInstances->GetInstanceCount();
    TreeInstances->BuildTreeIfOutdated(true, false);
    UE_LOG(LogTemp, Log, TEXT("InterVerseSG Quest foliage: %d HISM tree instances."), LastInstanceCount);
    return LastInstanceCount > 0;
}
