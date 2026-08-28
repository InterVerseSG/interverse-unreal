#include "InterVerseBuildingExtrusionActor.h"

#include "Algo/Reverse.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

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
    double X = 0.0, Y = 0.0;
    if (!(*Values)[0]->TryGetNumber(X) || !(*Values)[1]->TryGetNumber(Y))
    {
        return false;
    }
    OutPoint = FVector2D(static_cast<float>(X), static_cast<float>(Y));
    return true;
}

bool IsBuildingFeature(const TSharedPtr<FJsonObject>& Properties)
{
    if (!Properties.IsValid()) return false;
    const TSharedPtr<FJsonObject>* TagsPtr = nullptr;
    if (!Properties->TryGetObjectField(TEXT("osm_tags"), TagsPtr) || !TagsPtr || !TagsPtr->IsValid()) return false;
    FString BuildingValue;
    return (*TagsPtr)->TryGetStringField(TEXT("building"), BuildingValue)
        && !BuildingValue.IsEmpty() && BuildingValue != TEXT("no");
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

void AInterVerseBuildingExtrusionActor::BeginPlay()
{
    Super::BeginPlay();
    if (bEnableRuntimeSectorCulling && BuiltSectorCentersCm.Num() > 0)
    {
        UpdateSectorVisibility();
        GetWorldTimerManager().SetTimer(
            SectorVisibilityTimer,
            this,
            &AInterVerseBuildingExtrusionActor::UpdateSectorVisibility,
            SectorUpdateIntervalSeconds,
            true);
    }
}

void AInterVerseBuildingExtrusionActor::ClearBuildings()
{
    LastBuiltPolygonCount = 0;
    LastMeshSectionCount = 0;
    BuiltSectorIds.Reset();
    BuiltSectorCentersCm.Reset();
    if (ProceduralMesh) ProceduralMesh->ClearAllMeshSections();
}

bool AInterVerseBuildingExtrusionActor::RebuildBuildings()
{
    ClearBuildings();
    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), GeometryJsonRelativePath));
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: building geometry file not found: %s"), *AbsolutePath);
        return false;
    }
    const bool bBuilt = ParseAndBuild(JsonText);
    if (bBuilt)
    {
        UE_LOG(LogTemp, Log, TEXT("InterVerseSG Quest sectors: %d building polygons in %d spatial mesh section(s)."), LastBuiltPolygonCount, LastMeshSectionCount);
    }
    return bBuilt;
}

bool AInterVerseBuildingExtrusionActor::LoadSectorDefinitions(TArray<FString>& OutIds, TArray<FVector2D>& OutCenters) const
{
    OutIds.Reset();
    OutCenters.Reset();
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), SectorJsonRelativePath));
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *Path)) return false;

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;
    const TArray<TSharedPtr<FJsonValue>>* Sectors = nullptr;
    if (!Root->TryGetArrayField(TEXT("sectors"), Sectors) || !Sectors) return false;

    for (const TSharedPtr<FJsonValue>& Value : *Sectors)
    {
        const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid()) continue;
        FString Id;
        double X = 0.0, Y = 0.0;
        if (!(*ObjPtr)->TryGetStringField(TEXT("id"), Id)) continue;
        if (!(*ObjPtr)->TryGetNumberField(TEXT("center_x_cm"), X) || !(*ObjPtr)->TryGetNumberField(TEXT("center_y_cm"), Y)) continue;
        OutIds.Add(Id);
        OutCenters.Add(FVector2D(static_cast<float>(X), static_cast<float>(Y)));
    }
    return OutCenters.Num() > 0;
}

int32 AInterVerseBuildingExtrusionActor::FindNearestSector(const FVector2D& Point, const TArray<FVector2D>& Centers) const
{
    int32 Best = INDEX_NONE;
    float BestDistSq = TNumericLimits<float>::Max();
    for (int32 Index = 0; Index < Centers.Num(); ++Index)
    {
        const float D = FVector2D::DistSquared(Point, Centers[Index]);
        if (D < BestDistSq) { BestDistSq = D; Best = Index; }
    }
    return Best;
}

void AInterVerseBuildingExtrusionActor::UpdateSectorVisibility()
{
    if (!ProceduralMesh || BuiltSectorCentersCm.Num() == 0) return;
    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) return;

    const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
    const FVector2D PlayerXY(Local.X, Local.Y);
    const float RadiusSq = ActiveSectorRadiusCm * ActiveSectorRadiusCm;
    int32 Nearest = INDEX_NONE;
    float NearestDistSq = TNumericLimits<float>::Max();
    bool bAnyVisible = false;

    for (int32 Section = 0; Section < BuiltSectorCentersCm.Num(); ++Section)
    {
        const float DistSq = FVector2D::DistSquared(PlayerXY, BuiltSectorCentersCm[Section]);
        if (DistSq < NearestDistSq) { NearestDistSq = DistSq; Nearest = Section; }
        const bool bVisible = !bEnableRuntimeSectorCulling || DistSq <= RadiusSq;
        ProceduralMesh->SetMeshSectionVisible(Section, bVisible);
        bAnyVisible |= bVisible;
    }
    if (!bAnyVisible && Nearest != INDEX_NONE)
    {
        ProceduralMesh->SetMeshSectionVisible(Nearest, true);
    }
}

float AInterVerseBuildingExtrusionActor::ResolveHeightCm(const TSharedPtr<FJsonObject>& Properties) const
{
    if (!Properties.IsValid()) return DefaultBuildingHeightCm;
    const TSharedPtr<FJsonObject>* TagsPtr = nullptr;
    if (!Properties->TryGetObjectField(TEXT("osm_tags"), TagsPtr) || !TagsPtr || !TagsPtr->IsValid()) return DefaultBuildingHeightCm;
    const TSharedPtr<FJsonObject>& Tags = *TagsPtr;
    FString HeightText;
    if (Tags->TryGetStringField(TEXT("height"), HeightText))
    {
        HeightText.ReplaceInline(TEXT("m"), TEXT(""), ESearchCase::IgnoreCase);
        HeightText.TrimStartAndEndInline();
        const float HeightMeters = FCString::Atof(*HeightText);
        if (HeightMeters > 0.0f) return HeightMeters * 100.0f;
    }
    FString LevelsText;
    if (Tags->TryGetStringField(TEXT("building:levels"), LevelsText))
    {
        const float Levels = FCString::Atof(*LevelsText);
        if (Levels > 0.0f) return Levels * FloorHeightCm;
    }
    return DefaultBuildingHeightCm;
}

float AInterVerseBuildingExtrusionActor::ResolveBaseZCm(const TSharedPtr<FJsonObject>& Properties) const
{
    if (!Properties.IsValid()) return 0.0f;
    double BaseZ = 0.0;
    return Properties->TryGetNumberField(TEXT("terrain_base_z_cm"), BaseZ) ? static_cast<float>(BaseZ) : 0.0f;
}

bool AInterVerseBuildingExtrusionActor::ParseAndBuild(const FString& JsonText)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;
    const TArray<TSharedPtr<FJsonValue>>* Features = nullptr;
    if (!Root->TryGetArrayField(TEXT("features"), Features) || !Features) return false;

    TArray<FString> SectorIds;
    TArray<FVector2D> SectorCenters;
    if (!LoadSectorDefinitions(SectorIds, SectorCenters))
    {
        SectorIds = {TEXT("SECTOR_GLOBAL")};
        SectorCenters = {FVector2D::ZeroVector};
    }

    TArray<TArray<FVector>> SectorVertices;
    TArray<TArray<int32>> SectorTriangles;
    SectorVertices.SetNum(SectorCenters.Num());
    SectorTriangles.SetNum(SectorCenters.Num());
    int32 BuiltPolygonCount = 0;

    auto AppendRing = [&](const TArray<TSharedPtr<FJsonValue>>& RingValues, float BaseZCm, float HeightCm)
    {
        TArray<FVector2D> Polygon;
        for (const TSharedPtr<FJsonValue>& PointValue : RingValues)
        {
            FVector2D Point;
            if (ReadPoint2D(PointValue, Point)) Polygon.Add(Point);
        }
        if (Polygon.Num() > 2 && Polygon[0].Equals(Polygon.Last(), 0.1f)) Polygon.Pop();
        if (Polygon.Num() < 3) return;
        FVector2D Centroid = FVector2D::ZeroVector;
        for (const FVector2D& P : Polygon) Centroid += P;
        Centroid /= static_cast<float>(Polygon.Num());
        const int32 Sector = FindNearestSector(Centroid, SectorCenters);
        if (Sector != INDEX_NONE && AppendPolygonGeometry(Polygon, BaseZCm, HeightCm, SectorVertices[Sector], SectorTriangles[Sector])) ++BuiltPolygonCount;
    };

    for (const TSharedPtr<FJsonValue>& FeatureValue : *Features)
    {
        const TSharedPtr<FJsonObject>* FeaturePtr = nullptr;
        if (!FeatureValue.IsValid() || !FeatureValue->TryGetObject(FeaturePtr) || !FeaturePtr || !FeaturePtr->IsValid()) continue;
        const TSharedPtr<FJsonObject>& Feature = *FeaturePtr;
        const TSharedPtr<FJsonObject>* PropertiesPtr = nullptr;
        Feature->TryGetObjectField(TEXT("properties"), PropertiesPtr);
        const TSharedPtr<FJsonObject> Properties = (PropertiesPtr && PropertiesPtr->IsValid()) ? *PropertiesPtr : nullptr;
        if (!IsBuildingFeature(Properties)) continue;
        const TSharedPtr<FJsonObject>* GeometryPtr = nullptr;
        if (!Feature->TryGetObjectField(TEXT("geometry"), GeometryPtr) || !GeometryPtr || !GeometryPtr->IsValid()) continue;
        const TSharedPtr<FJsonObject>& Geometry = *GeometryPtr;
        FString GeometryType;
        if (!Geometry->TryGetStringField(TEXT("type"), GeometryType) || (GeometryType != TEXT("Polygon") && GeometryType != TEXT("MultiPolygon"))) continue;
        const float HeightCm = ResolveHeightCm(Properties);
        const float BaseZCm = ResolveBaseZCm(Properties);
        const TArray<TSharedPtr<FJsonValue>>* Coordinates = nullptr;
        if (!Geometry->TryGetArrayField(TEXT("coordinates_cm"), Coordinates) || !Coordinates) continue;

        if (GeometryType == TEXT("Polygon") && Coordinates->Num() > 0)
        {
            const TArray<TSharedPtr<FJsonValue>>* OuterRing = nullptr;
            if ((*Coordinates)[0]->TryGetArray(OuterRing) && OuterRing) AppendRing(*OuterRing, BaseZCm, HeightCm);
        }
        else if (GeometryType == TEXT("MultiPolygon"))
        {
            for (const TSharedPtr<FJsonValue>& PolygonValue : *Coordinates)
            {
                const TArray<TSharedPtr<FJsonValue>>* Rings = nullptr;
                if (!PolygonValue->TryGetArray(Rings) || !Rings || Rings->Num() == 0) continue;
                const TArray<TSharedPtr<FJsonValue>>* OuterRing = nullptr;
                if ((*Rings)[0]->TryGetArray(OuterRing) && OuterRing) AppendRing(*OuterRing, BaseZCm, HeightCm);
            }
        }
    }

    int32 SectionIndex = 0;
    for (int32 Sector = 0; Sector < SectorCenters.Num(); ++Sector)
    {
        if (SectorVertices[Sector].Num() == 0 || SectorTriangles[Sector].Num() == 0) continue;
        TArray<FVector> Normals;
        TArray<FVector2D> UV0;
        TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;
        ProceduralMesh->CreateMeshSection_LinearColor(SectionIndex, SectorVertices[Sector], SectorTriangles[Sector], Normals, UV0, Colors, Tangents, bCreateCollision);
        BuiltSectorIds.Add(SectorIds.IsValidIndex(Sector) ? SectorIds[Sector] : FString::Printf(TEXT("SECTOR_%d"), Sector));
        BuiltSectorCentersCm.Add(SectorCenters[Sector]);
        ++SectionIndex;
    }

    LastBuiltPolygonCount = BuiltPolygonCount;
    LastMeshSectionCount = SectionIndex;
    return BuiltPolygonCount > 0 && SectionIndex > 0;
}

bool AInterVerseBuildingExtrusionActor::AppendPolygonGeometry(const TArray<FVector2D>& InputPolygon, float BaseZCm, float HeightCm, TArray<FVector>& Vertices, TArray<int32>& Triangles) const
{
    TArray<FVector2D> Polygon = InputPolygon;
    if (Polygon.Num() < 3) return false;
    if (SignedArea(Polygon) < 0.0f) Algo::Reverse(Polygon);
    TArray<int32> RoofTriangles;
    if (!TriangulatePolygon(Polygon, RoofTriangles)) return false;
    const int32 Count = Polygon.Num();
    const int32 VertexBase = Vertices.Num();
    for (const FVector2D& Point : Polygon) Vertices.Add(FVector(Point.X, Point.Y, BaseZCm));
    for (const FVector2D& Point : Polygon) Vertices.Add(FVector(Point.X, Point.Y, BaseZCm + HeightCm));
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const int32 Next = (Index + 1) % Count;
        const int32 B0 = VertexBase + Index, B1 = VertexBase + Next, T0 = VertexBase + Index + Count, T1 = VertexBase + Next + Count;
        Triangles.Append({B0, B1, T1, B0, T1, T0});
    }
    for (int32 Index = 0; Index < RoofTriangles.Num(); Index += 3)
    {
        Triangles.Add(VertexBase + RoofTriangles[Index] + Count);
        Triangles.Add(VertexBase + RoofTriangles[Index + 1] + Count);
        Triangles.Add(VertexBase + RoofTriangles[Index + 2] + Count);
    }
    return true;
}

bool AInterVerseBuildingExtrusionActor::TriangulatePolygon(const TArray<FVector2D>& Polygon, TArray<int32>& OutTriangles)
{
    OutTriangles.Reset();
    if (Polygon.Num() < 3) return false;
    TArray<int32> Remaining;
    for (int32 Index = 0; Index < Polygon.Num(); ++Index) Remaining.Add(Index);
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
            if (Cross2D(A, B, C) <= KINDA_SMALL_NUMBER) continue;
            bool bContainsOtherPoint = false;
            for (const int32 TestIndex : Remaining)
            {
                if (TestIndex == PrevIndex || TestIndex == CurrIndex || TestIndex == NextIndex) continue;
                if (PointInTriangle(Polygon[TestIndex], A, B, C)) { bContainsOtherPoint = true; break; }
            }
            if (!bContainsOtherPoint)
            {
                OutTriangles.Append({PrevIndex, CurrIndex, NextIndex});
                Remaining.RemoveAt(LocalIndex);
                bClippedEar = true;
                break;
            }
        }
        if (!bClippedEar) return false;
    }
    if (Remaining.Num() == 3)
    {
        OutTriangles.Append({Remaining[0], Remaining[1], Remaining[2]});
        return true;
    }
    return false;
}
