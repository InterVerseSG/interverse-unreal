#include "InterVerseCampusPropsActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AInterVerseCampusPropsActor::AInterVerseCampusPropsActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BenchStandard = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BenchStandard"));
    BenchPriority = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BenchPriority"));
    LampStandard = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LampStandard"));
    LampPriority = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LampPriority"));
    SignStandard = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("SignStandard"));
    SignPriority = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("SignPriority"));

    for (UHierarchicalInstancedStaticMeshComponent* Component : {
        BenchStandard.Get(), BenchPriority.Get(), LampStandard.Get(), LampPriority.Get(), SignStandard.Get(), SignPriority.Get()})
    {
        Component->SetupAttachment(SceneRoot);
        Component->SetMobility(EComponentMobility::Static);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
    }
}

void AInterVerseCampusPropsActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
#if WITH_EDITOR
    RebuildProps();
#endif
}

void AInterVerseCampusPropsActor::ConfigureHISM(
    UHierarchicalInstancedStaticMeshComponent* Component,
    UStaticMesh* Mesh,
    bool bPriority) const
{
    if (!Component)
    {
        return;
    }

    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCanEverAffectNavigation(false);
    Component->SetCastShadow(bCastShadows);
    Component->bCastDynamicShadow = bCastShadows;
    Component->bCastStaticShadow = bCastShadows;
    Component->SetCullDistances(
        bPriority ? PriorityStartCullDistanceCm : StandardStartCullDistanceCm,
        bPriority ? PriorityEndCullDistanceCm : StandardEndCullDistanceCm);
}

void AInterVerseCampusPropsActor::ClearProps()
{
    LastBenchCount = 0;
    LastLampCount = 0;
    LastSignCount = 0;

    for (UHierarchicalInstancedStaticMeshComponent* Component : {
        BenchStandard.Get(), BenchPriority.Get(), LampStandard.Get(), LampPriority.Get(), SignStandard.Get(), SignPriority.Get()})
    {
        if (Component)
        {
            Component->ClearInstances();
        }
    }
}

bool AInterVerseCampusPropsActor::RebuildProps()
{
    ClearProps();

    ConfigureHISM(BenchStandard, BenchMesh, false);
    ConfigureHISM(BenchPriority, BenchMesh, true);
    ConfigureHISM(LampStandard, LampMesh, false);
    ConfigureHISM(LampPriority, LampMesh, true);
    ConfigureHISM(SignStandard, SignMesh, false);
    ConfigureHISM(SignPriority, SignMesh, true);

    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), PropsJsonRelativePath));

    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *AbsolutePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: campus props file not found: %s"), *AbsolutePath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: invalid campus props JSON."));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Objects = nullptr;
    if (!Root->TryGetArrayField(TEXT("objects"), Objects) || !Objects)
    {
        return false;
    }

    int32 Added = 0;
    for (const TSharedPtr<FJsonValue>& Value : *Objects)
    {
        const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid())
        {
            continue;
        }

        const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
        FString Category;
        double X = 0.0, Y = 0.0, Z = 0.0;
        bool bPriority = false;
        if (!Obj->TryGetStringField(TEXT("category"), Category)
            || !Obj->TryGetNumberField(TEXT("x_cm"), X)
            || !Obj->TryGetNumberField(TEXT("y_cm"), Y))
        {
            continue;
        }
        Obj->TryGetNumberField(TEXT("z_cm"), Z);
        Obj->TryGetBoolField(TEXT("priority"), bPriority);

        UHierarchicalInstancedStaticMeshComponent* Target = nullptr;
        if (Category == TEXT("bench"))
        {
            Target = bPriority ? BenchPriority.Get() : BenchStandard.Get();
            ++LastBenchCount;
        }
        else if (Category == TEXT("street_lamp"))
        {
            Target = bPriority ? LampPriority.Get() : LampStandard.Get();
            ++LastLampCount;
        }
        else if (Category == TEXT("sign"))
        {
            Target = bPriority ? SignPriority.Get() : SignStandard.Get();
            ++LastSignCount;
        }

        if (!Target || !Target->GetStaticMesh())
        {
            continue;
        }

        Target->AddInstance(FTransform(FRotator::ZeroRotator, FVector(
            static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z))));
        ++Added;
    }

    UE_LOG(LogTemp, Log,
        TEXT("InterVerseSG Quest props: %d mapped objects instanced (benches=%d, lamps=%d, signs=%d)."),
        Added, LastBenchCount, LastLampCount, LastSignCount);

    return Added > 0;
}
