#include "InterVerseBuildingExtrusionActor.h"

#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
float SignedArea(const TArray<FVector2D>& Points)
{
    double Area = 0.0;
    const int32 Count = Points.Num();
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector2D& A = Points[Index];
        const FVector2D& B = Points[(Index + 1) % Count];
        Area += static_cast<double>(A.X) * B.Y - static_cast<double>(B.X) * A.Y;
    }
    return static_cast<float>(Area * 0.5);
}

float Cross2D(const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
    const FVector2D AB = B - A;
    const FVector2D AC = C - A;
    return AB.X * AC.Y - AB.Y * AC.X;
}

bool PointInTriangle(const FVector2D& P, const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
    const float C1 = Cross2D(A, B, P);
    const float C2 = Cross2D(B, C, P);
    const float C3 = Cross2D(C, A, P);
    const bool bHasNegative = (C1 < 0.0f) || (C2 < 0.0f) || (C3 < 0.0f);
    const bool bHasPositive = (C1 > 0.0f) || (C2 > 0.0f) || (C3 > 0.0f);
    return !(bHasNegative && bHasPositive);
}

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

AInterVerseBuildingExtrusionActor::AInterVerseBuildingExtrusionActor()
{
    PrimaryActorTick.bCanEverTick = false;

    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    SetRootComponent(ProceduralMesh);
    ProceduralMesh->SetMobility(EComponentMobility::Static);
}

void AInterVerseBuildingExtrusionActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
#if WITH_EDITOR
    RebuildBuildings();
#endif
}

void AInterVerseBuildingExtrusionActor::ClearBuildings()
{
    if (ProceduralMesh)
    {
        ProceduralMesh->ClearAllMeshSections();
    }
}

bool AInterVerseBuildingExtrusionActor::RebuildBuildings()
{
    ClearBuildings();

    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), GeometryJsonRelativePath)
    );

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: building geometry file not found: %s"), *AbsolutePath);
        return false;
    }

    const bool bBuilt = ParseAndBuild(JsonText);
    if (bBuilt)
    {
        UE_LOG(LogTemp, Log, TEXT("InterVerseSG: procedural campus buildings rebuilt from %s"), *AbsolutePath);
    }
    return bBuilt;
}

float AInterVerseBuildingExtrusionActor::ResolveHeightCm(const TSharedPtr<FJsonObject>& Properties) const
{
    if (!Properties.IsValid())
    {
        return DefaultBuildingHeightCm;
    }

    const TSharedPtr<FJsonObject>* TagsPtr = nullptr;
    if (!Properties->TryGetObjectField(TEXT("osm_tags"), TagsPtr) || !TagsPtr || !TagsPtr->IsValid())
    {
        return DefaultBuildingHeightCm;
    }

    const TSharedPtr<FJsonObject>& Tags = *TagsPtr;
    FString HeightText;
    if (Tags->TryGetStringField(TEXT("height"), HeightText))
    {
        HeightText.ReplaceInline(TEXT("m"), TEXT(""), ESearchCase::IgnoreCase);
        HeightText.TrimStartAndEndInline();
        const float HeightMeters = FCString::Atof(*HeightText);
        if (HeightMeters > 0.0f)
        {
            return HeightMeters * 100.0f;
        }
    }

    FString LevelsText;
    if (Tags->TryGetStringField(TEXT("building:levels"), LevelsText))
    {
        const float Levels = FCString::Atof(*LevelsText);
        if (Levels > 0.0f)
        {
            return Levels * FloorHeightCm;
        }
    }

    return DefaultBuildingHeightCm;
}

bool AInterVerseBuildingExtrusionActor::ParseAndBuild(const FString& JsonText)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: invalid campus geometry JSON."));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Features = nullptr;
    if (!Root->TryGetArrayField(TEXT("features"), Features) || !Features)
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: campus geometry JSON has no features array."));
        return false;
    }

    int32 SectionIndex = 0;
    int32 BuiltPolygonCount = 0;

    for (const TSharedPtr<FJsonValue>& FeatureValue : *Features)
    {
        const TSharedPtr<FJsonObject>* FeaturePtr = nullptr;
        if (!FeatureValue.IsValid() || !FeatureValue->TryGetObject(FeaturePtr) || !FeaturePtr || !FeaturePtr->IsValid())
        {
            continue;
        }

        const TSharedPtr<FJsonObject>& Feature = *FeaturePtr;
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

        if (GeometryType != TEXT("Polygon") && GeometryType != TEXT("MultiPolygon"))
        {
            continue;
        }

        const TSharedPtr<FJsonObject>* PropertiesPtr = nullptr;
        Feature->TryGetObjectField(TEXT("properties"), PropertiesPtr);
        const float HeightCm = ResolveHeightCm(PropertiesPtr ? *PropertiesPtr : nullptr);

        const TArray<TSharedPtr<FJsonValue>>* Coordinates = nullptr;
        if (!Geometry->TryGetArrayField(TEXT("coordinates_cm"), Coordinates) || !Coordinates)
        {
            continue;
        }

        auto BuildRing = [&](const TArray<TSharedPtr<FJsonValue>>& RingValues)
        {
            TArray<FVector2D> Polygon;
            for (const TSharedPtr<FJsonValue>& PointValue : RingValues)
            {
                FVector2D Point;
                if (ReadPoint2D(PointValue, Point))
                {
                    Polygon.Add(Point);
                }
            }

            if (Polygon.Num() > 2 && Polygon[0].Equals(Polygon.Last(), 0.1f))
            {
                Polygon.Pop();
            }

            if (Polygon.Num() >= 3 && BuildPolygonSection(Polygon, HeightCm, SectionIndex))
            {
                ++SectionIndex;
                ++BuiltPolygonCount;
            }
        };

        if (GeometryType == TEXT("Polygon"))
        {
            if (Coordinates->Num() > 0)
            {
                const TArray<TSharedPtr<FJsonValue>>* OuterRing = nullptr;
                if ((*Coordinates)[0]->TryGetArray(OuterRing) && OuterRing)
                {
                    BuildRing(*OuterRing);
                }
            }
        }
        else
        {
            for (const TSharedPtr<FJsonValue>& PolygonValue : *Coordinates)
            {
                const TArray<TSharedPtr<FJsonValue>>* Rings = nullptr;
                if (!PolygonValue->TryGetArray(Rings) || !Rings || Rings->Num() == 0)
                {
                    continue;
                }
                const TArray<TSharedPtr<FJsonValue>>* OuterRing = nullptr;
                if ((*Rings)[0]->TryGetArray(OuterRing) && OuterRing)
                {
                    BuildRing(*OuterRing);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("InterVerseSG: built %d footprint extrusions."), BuiltPolygonCount);
    return BuiltPolygonCount > 0;
}

bool AInterVerseBuildingExtrusionActor::BuildPolygonSection(
    const TArray<FVector2D>& InputPolygon,
    float HeightCm,
    int32 SectionIndex)
{
    TArray<FVector2D> Polygon = InputPolygon;
    if (Polygon.Num() < 3)
    {
        return false;
    }

    if (SignedArea(Polygon) < 0.0f)
    {
        Algo::Reverse(Polygon);
    }

    TArray<int32> RoofTriangles;
    if (!TriangulatePolygon(Polygon, RoofTriangles))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: could not triangulate footprint section %d."), SectionIndex);
        return false;
    }

    const int32 Count = Polygon.Num();
    TArray<FVector> Vertices;
    Vertices.Reserve(Count * 2);
    for (const FVector2D& Point : Polygon)
    {
        Vertices.Add(FVector(Point.X, Point.Y, 0.0f));
    }
    for (const FVector2D& Point : Polygon)
    {
        Vertices.Add(FVector(Point.X, Point.Y, HeightCm));
    }

    TArray<int32> Triangles;
    Triangles.Reserve(RoofTriangles.Num() + Count * 6);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const int32 Next = (Index + 1) % Count;
        const int32 B0 = Index;
        const int32 B1 = Next;
        const int32 T0 = Index + Count;
        const int32 T1 = Next + Count;

        Triangles.Append({B0, B1, T1, B0, T1, T0});
    }

    for (int32 Index = 0; Index < RoofTriangles.Num(); Index += 3)
    {
        Triangles.Add(RoofTriangles[Index] + Count);
        Triangles.Add(RoofTriangles[Index + 1] + Count);
        Triangles.Add(RoofTriangles[Index + 2] + Count);
    }

    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    ProceduralMesh->CreateMeshSection_LinearColor(
        SectionIndex,
        Vertices,
        Triangles,
        Normals,
        UV0,
        VertexColors,
        Tangents,
        bCreateCollision
    );

    return true;
}

bool AInterVerseBuildingExtrusionActor::TriangulatePolygon(
    const TArray<FVector2D>& Polygon,
    TArray<int32>& OutTriangles)
{
    OutTriangles.Reset();
    if (Polygon.Num() < 3)
    {
        return false;
    }

    TArray<int32> Remaining;
    Remaining.Reserve(Polygon.Num());
    for (int32 Index = 0; Index < Polygon.Num(); ++Index)
    {
        Remaining.Add(Index);
    }

    int32 SafetyCounter = 0;
    const int32 MaxIterations = Polygon.Num() * Polygon.Num();

    while (Remaining.Num() > 3 && SafetyCounter++ < MaxIterations)
    {
        bool bClippedEar = false;

        for (int32 LocalIndex = 0; LocalIndex < Remaining.Num(); ++LocalIndex)
        {
            const int32 PrevIndex = Remaining[(LocalIndex - 1 + Remaining.Num()) % Remaining.Num()];
            const int32 CurrIndex = Remaining[LocalIndex];
            const int32 NextIndex = Remaining[(LocalIndex + 1) % Remaining.Num()];

            const FVector2D& A = Polygon[PrevIndex];
            const FVector2D& B = Polygon[CurrIndex];
            const FVector2D& C = Polygon[NextIndex];

            if (Cross2D(A, B, C) <= KINDA_SMALL_NUMBER)
            {
                continue;
            }

            bool bContainsOtherPoint = false;
            for (const int32 TestIndex : Remaining)
            {
                if (TestIndex == PrevIndex || TestIndex == CurrIndex || TestIndex == NextIndex)
                {
                    continue;
                }

                if (PointInTriangle(Polygon[TestIndex], A, B, C))
                {
                    bContainsOtherPoint = true;
                    break;
                }
            }

            if (bContainsOtherPoint)
            {
                continue;
            }

            OutTriangles.Append({PrevIndex, CurrIndex, NextIndex});
            Remaining.RemoveAt(LocalIndex);
            bClippedEar = true;
            break;
        }

        if (!bClippedEar)
        {
            return false;
        }
    }

    if (Remaining.Num() == 3)
    {
        OutTriangles.Append({Remaining[0], Remaining[1], Remaining[2]});
        return true;
    }

    return false;
}
