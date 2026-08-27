#include "InterVerseCampusSurfaceActor.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadPoint2D(const TSharedPtr<FJsonValue>& Value, FVector2D& OutPoint)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Value.IsValid() || !Value->TryGetArray(Values) || !Values || Values->Num() < 2)
    {
        return false;
    }

    double X = 0.0;
    double Y = 0.0;
    if (!(*Values)[0]->TryGetNumber(X) || !(*Values)[1]->TryGetNumber(Y))
    {
        return false;
    }

    OutPoint = FVector2D(static_cast<float>(X), static_cast<float>(Y));
    return true;
}
}

AInterVerseCampusSurfaceActor::AInterVerseCampusSurfaceActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SurfaceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SurfaceMesh"));
    SetRootComponent(SurfaceMesh);
    SurfaceMesh->SetMobility(EComponentMobility::Static);
}

void AInterVerseCampusSurfaceActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
#if WITH_EDITOR
    RebuildSurfaces();
#endif
}

void AInterVerseCampusSurfaceActor::ClearSurfaces()
{
    if (SurfaceMesh)
    {
        SurfaceMesh->ClearAllMeshSections();
    }
}

bool AInterVerseCampusSurfaceActor::RebuildSurfaces()
{
    ClearSurfaces();

    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), SurfacesJsonRelativePath)
    );

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: campus surface file not found: %s"), *AbsolutePath);
        return false;
    }

    const bool bBuilt = ParseAndBuild(JsonText);
    if (bBuilt)
    {
        UE_LOG(LogTemp, Log, TEXT("InterVerseSG: campus circulation surfaces rebuilt from %s"), *AbsolutePath);
    }
    return bBuilt;
}

float AInterVerseCampusSurfaceActor::WidthForCategory(const FString& Category) const
{
    if (Category.Equals(TEXT("road"), ESearchCase::IgnoreCase))
    {
        return RoadWidthCm;
    }
    if (Category.Equals(TEXT("parking"), ESearchCase::IgnoreCase))
    {
        return ParkingOutlineWidthCm;
    }
    return PedestrianWidthCm;
}

void AInterVerseCampusSurfaceActor::AddPolylineRibbon(
    const TArray<FVector2D>& Points,
    float WidthCm,
    TArray<FVector>& Vertices,
    TArray<int32>& Triangles) const
{
    if (Points.Num() < 2)
    {
        return;
    }

    const float HalfWidth = WidthCm * 0.5f;

    for (int32 Index = 0; Index < Points.Num() - 1; ++Index)
    {
        const FVector2D A = Points[Index];
        const FVector2D B = Points[Index + 1];
        FVector2D Direction = B - A;
        const float Length = Direction.Size();
        if (Length <= KINDA_SMALL_NUMBER)
        {
            continue;
        }
        Direction /= Length;
        const FVector2D Normal(-Direction.Y, Direction.X);

        const FVector2D AL = A + Normal * HalfWidth;
        const FVector2D AR = A - Normal * HalfWidth;
        const FVector2D BL = B + Normal * HalfWidth;
        const FVector2D BR = B - Normal * HalfWidth;

        const int32 Base = Vertices.Num();
        Vertices.Add(FVector(AL.X, AL.Y, SurfaceZCm));
        Vertices.Add(FVector(AR.X, AR.Y, SurfaceZCm));
        Vertices.Add(FVector(BL.X, BL.Y, SurfaceZCm));
        Vertices.Add(FVector(BR.X, BR.Y, SurfaceZCm));

        Triangles.Append({Base, Base + 2, Base + 1, Base + 1, Base + 2, Base + 3});
    }
}

bool AInterVerseCampusSurfaceActor::ParseAndBuild(const FString& JsonText)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: invalid campus surface JSON."));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Features = nullptr;
    if (!Root->TryGetArrayField(TEXT("features"), Features) || !Features)
    {
        return false;
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    int32 FeatureCount = 0;

    for (const TSharedPtr<FJsonValue>& FeatureValue : *Features)
    {
        const TSharedPtr<FJsonObject>* FeaturePtr = nullptr;
        if (!FeatureValue.IsValid() || !FeatureValue->TryGetObject(FeaturePtr) || !FeaturePtr || !FeaturePtr->IsValid())
        {
            continue;
        }

        const TSharedPtr<FJsonObject>& Feature = *FeaturePtr;
        FString Category = TEXT("pedestrian");
        const TSharedPtr<FJsonObject>* PropertiesPtr = nullptr;
        if (Feature->TryGetObjectField(TEXT("properties"), PropertiesPtr) && PropertiesPtr && PropertiesPtr->IsValid())
        {
            (*PropertiesPtr)->TryGetStringField(TEXT("category"), Category);
        }

        const TSharedPtr<FJsonObject>* GeometryPtr = nullptr;
        if (!Feature->TryGetObjectField(TEXT("geometry"), GeometryPtr) || !GeometryPtr || !GeometryPtr->IsValid())
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
        if (!Geometry->TryGetArrayField(TEXT("coordinates_cm"), Coordinates) || !Coordinates)
        {
            continue;
        }

        const float Width = WidthForCategory(Category);

        if (GeometryType == TEXT("LineString"))
        {
            TArray<FVector2D> Points;
            for (const TSharedPtr<FJsonValue>& PointValue : *Coordinates)
            {
                FVector2D P;
                if (ReadPoint2D(PointValue, P))
                {
                    Points.Add(P);
                }
            }
            AddPolylineRibbon(Points, Width, Vertices, Triangles);
            ++FeatureCount;
        }
        else if (GeometryType == TEXT("Polygon") && Coordinates->Num() > 0)
        {
            const TArray<TSharedPtr<FJsonValue>>* OuterRing = nullptr;
            if ((*Coordinates)[0]->TryGetArray(OuterRing) && OuterRing)
            {
                TArray<FVector2D> Points;
                for (const TSharedPtr<FJsonValue>& PointValue : *OuterRing)
                {
                    FVector2D P;
                    if (ReadPoint2D(PointValue, P))
                    {
                        Points.Add(P);
                    }
                }
                AddPolylineRibbon(Points, Width, Vertices, Triangles);
                ++FeatureCount;
            }
        }
    }

    if (Vertices.Num() == 0 || Triangles.Num() == 0)
    {
        return false;
    }

    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    SurfaceMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UV0,
        Colors,
        Tangents,
        bCreateCollision
    );

    UE_LOG(LogTemp, Log, TEXT("InterVerseSG: built %d campus circulation/surface features."), FeatureCount);
    return true;
}
