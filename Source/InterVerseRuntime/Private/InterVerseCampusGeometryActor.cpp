#include "InterVerseCampusGeometryActor.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    static const FName GeneratedGeometryTag(TEXT("InterVerseGeneratedGeometry"));

    bool JsonPointToVector(const TSharedPtr<FJsonValue>& Value, FVector& OutVector, float ZOffsetCm)
    {
        const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
        if (!Value.IsValid() || !Value->TryGetArray(Array) || Array == nullptr || Array->Num() < 2)
        {
            return false;
        }

        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;
        if (!(*Array)[0]->TryGetNumber(X) || !(*Array)[1]->TryGetNumber(Y))
        {
            return false;
        }
        if (Array->Num() >= 3)
        {
            (*Array)[2]->TryGetNumber(Z);
        }

        OutVector = FVector(X, Y, Z + ZOffsetCm);
        return true;
    }
}

AInterVerseCampusGeometryActor::AInterVerseCampusGeometryActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void AInterVerseCampusGeometryActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildGeometry();
}

void AInterVerseCampusGeometryActor::ClearGeneratedSplines()
{
    TArray<USplineComponent*> SplineComponents;
    GetComponents<USplineComponent>(SplineComponents);

    for (USplineComponent* Spline : SplineComponents)
    {
        if (Spline && Spline->ComponentTags.Contains(GeneratedGeometryTag))
        {
            Spline->DestroyComponent();
        }
    }
}

void AInterVerseCampusGeometryActor::AddSplineFromPoints(
    const TArray<FVector>& Points,
    bool bClosedLoop,
    const FString& FeatureId)
{
    if (Points.Num() < 2)
    {
        return;
    }

    USplineComponent* Spline = NewObject<USplineComponent>(this);
    Spline->SetupAttachment(SceneRoot);
    Spline->RegisterComponent();
    AddInstanceComponent(Spline);
    Spline->ComponentTags.Add(GeneratedGeometryTag);
    Spline->ComponentTags.Add(FName(*FeatureId));
    Spline->ClearSplinePoints(false);

    for (const FVector& Point : Points)
    {
        Spline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
    }

    Spline->SetClosedLoop(bClosedLoop, false);
    Spline->UpdateSpline();
}

void AInterVerseCampusGeometryActor::RebuildGeometry()
{
    ClearGeneratedSplines();

    const FString Path = FPaths::Combine(FPaths::ProjectConfigDir(), GeometryConfigRelativePath);
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *Path))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG geometry file not found: %s"), *Path);
        return;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG could not parse geometry JSON: %s"), *Path);
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Features = nullptr;
    if (!RootObject->TryGetArrayField(TEXT("features"), Features) || Features == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG geometry JSON contains no features."));
        return;
    }

    int32 GeneratedCount = 0;

    for (const TSharedPtr<FJsonValue>& FeatureValue : *Features)
    {
        const TSharedPtr<FJsonObject>* FeatureObjectPtr = nullptr;
        if (!FeatureValue.IsValid() || !FeatureValue->TryGetObject(FeatureObjectPtr) || FeatureObjectPtr == nullptr)
        {
            continue;
        }

        const TSharedPtr<FJsonObject>& FeatureObject = *FeatureObjectPtr;
        FString FeatureId(TEXT("UnnamedFeature"));
        const TSharedPtr<FJsonObject>* PropertiesPtr = nullptr;
        if (FeatureObject->TryGetObjectField(TEXT("properties"), PropertiesPtr) && PropertiesPtr != nullptr)
        {
            (*PropertiesPtr)->TryGetStringField(TEXT("id"), FeatureId);
        }

        const TSharedPtr<FJsonObject>* GeometryPtr = nullptr;
        if (!FeatureObject->TryGetObjectField(TEXT("geometry"), GeometryPtr) || GeometryPtr == nullptr)
        {
            continue;
        }

        const TSharedPtr<FJsonObject>& Geometry = *GeometryPtr;
        FString GeometryType;
        if (!Geometry->TryGetStringField(TEXT("type"), GeometryType))
        {
            continue;
        }

        const TArray<TSharedPtr<FJsonValue>>* Coordinates = nullptr;
        if (!Geometry->TryGetArrayField(TEXT("coordinates_cm"), Coordinates) || Coordinates == nullptr)
        {
            continue;
        }

        if (GeometryType == TEXT("LineString"))
        {
            TArray<FVector> Points;
            for (const TSharedPtr<FJsonValue>& PointValue : *Coordinates)
            {
                FVector Point;
                if (JsonPointToVector(PointValue, Point, ZOffsetCm))
                {
                    Points.Add(Point);
                }
            }
            AddSplineFromPoints(Points, false, FeatureId);
            ++GeneratedCount;
        }
        else if (GeometryType == TEXT("Polygon"))
        {
            for (const TSharedPtr<FJsonValue>& RingValue : *Coordinates)
            {
                const TArray<TSharedPtr<FJsonValue>>* Ring = nullptr;
                if (!RingValue.IsValid() || !RingValue->TryGetArray(Ring) || Ring == nullptr)
                {
                    continue;
                }

                TArray<FVector> Points;
                for (const TSharedPtr<FJsonValue>& PointValue : *Ring)
                {
                    FVector Point;
                    if (JsonPointToVector(PointValue, Point, ZOffsetCm))
                    {
                        Points.Add(Point);
                    }
                }
                AddSplineFromPoints(Points, bClosedBuildingLoops, FeatureId);
                ++GeneratedCount;
            }
        }
        else if (GeometryType == TEXT("MultiLineString"))
        {
            for (const TSharedPtr<FJsonValue>& LineValue : *Coordinates)
            {
                const TArray<TSharedPtr<FJsonValue>>* Line = nullptr;
                if (!LineValue.IsValid() || !LineValue->TryGetArray(Line) || Line == nullptr)
                {
                    continue;
                }
                TArray<FVector> Points;
                for (const TSharedPtr<FJsonValue>& PointValue : *Line)
                {
                    FVector Point;
                    if (JsonPointToVector(PointValue, Point, ZOffsetCm))
                    {
                        Points.Add(Point);
                    }
                }
                AddSplineFromPoints(Points, false, FeatureId);
                ++GeneratedCount;
            }
        }
        else if (GeometryType == TEXT("MultiPolygon"))
        {
            for (const TSharedPtr<FJsonValue>& PolygonValue : *Coordinates)
            {
                const TArray<TSharedPtr<FJsonValue>>* Polygon = nullptr;
                if (!PolygonValue.IsValid() || !PolygonValue->TryGetArray(Polygon) || Polygon == nullptr)
                {
                    continue;
                }
                for (const TSharedPtr<FJsonValue>& RingValue : *Polygon)
                {
                    const TArray<TSharedPtr<FJsonValue>>* Ring = nullptr;
                    if (!RingValue.IsValid() || !RingValue->TryGetArray(Ring) || Ring == nullptr)
                    {
                        continue;
                    }
                    TArray<FVector> Points;
                    for (const TSharedPtr<FJsonValue>& PointValue : *Ring)
                    {
                        FVector Point;
                        if (JsonPointToVector(PointValue, Point, ZOffsetCm))
                        {
                            Points.Add(Point);
                        }
                    }
                    AddSplineFromPoints(Points, bClosedBuildingLoops, FeatureId);
                    ++GeneratedCount;
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("InterVerseSG generated %d campus geometry spline(s)."), GeneratedCount);
}
