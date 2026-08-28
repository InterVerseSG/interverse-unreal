#include "InterVerseGreenAreaActor.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
float Cross2D(const FVector& A, const FVector& B, const FVector& C)
{
    const FVector2D AB(B.X - A.X, B.Y - A.Y);
    const FVector2D AC(C.X - A.X, C.Y - A.Y);
    return AB.X * AC.Y - AB.Y * AC.X;
}

bool PointInTriangle2D(const FVector& P, const FVector& A, const FVector& B, const FVector& C)
{
    const float C1 = Cross2D(A, B, P);
    const float C2 = Cross2D(B, C, P);
    const float C3 = Cross2D(C, A, P);
    const bool bNeg = C1 < 0 || C2 < 0 || C3 < 0;
    const bool bPos = C1 > 0 || C2 > 0 || C3 > 0;
    return !(bNeg && bPos);
}

bool ReadPoint3D(const TSharedPtr<FJsonValue>& Value, FVector& OutPoint)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Value.IsValid() || !Value->TryGetArray(Values) || !Values || Values->Num() < 2) return false;
    double X=0,Y=0,Z=0;
    if (!(*Values)[0]->TryGetNumber(X) || !(*Values)[1]->TryGetNumber(Y)) return false;
    if (Values->Num() > 2) (*Values)[2]->TryGetNumber(Z);
    OutPoint = FVector((float)X,(float)Y,(float)Z);
    return true;
}
}

AInterVerseGreenAreaActor::AInterVerseGreenAreaActor()
{
    PrimaryActorTick.bCanEverTick = false;
    GreenMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GreenMesh"));
    SetRootComponent(GreenMesh);
    GreenMesh->SetMobility(EComponentMobility::Static);
}

void AInterVerseGreenAreaActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
#if WITH_EDITOR
    RebuildGreenAreas();
#endif
}

void AInterVerseGreenAreaActor::ClearGreenAreas()
{
    LastPolygonCount = 0;
    if (GreenMesh) GreenMesh->ClearAllMeshSections();
}

bool AInterVerseGreenAreaActor::RebuildGreenAreas()
{
    ClearGreenAreas();
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), GreenAreaJsonRelativePath));
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *Path))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: green area file not found: %s"), *Path);
        return false;
    }
    return ParseAndBuild(Text);
}

bool AInterVerseGreenAreaActor::ParseAndBuild(const FString& JsonText)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;
    const TArray<TSharedPtr<FJsonValue>>* Features = nullptr;
    if (!Root->TryGetArrayField(TEXT("features"), Features) || !Features) return false;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    for (const TSharedPtr<FJsonValue>& FeatureValue : *Features)
    {
        const TSharedPtr<FJsonObject>* FeaturePtr = nullptr;
        if (!FeatureValue.IsValid() || !FeatureValue->TryGetObject(FeaturePtr) || !FeaturePtr || !FeaturePtr->IsValid()) continue;
        const TSharedPtr<FJsonObject>* GeometryPtr = nullptr;
        if (!(*FeaturePtr)->TryGetObjectField(TEXT("geometry"), GeometryPtr) || !GeometryPtr || !GeometryPtr->IsValid()) continue;
        FString Type;
        if (!(*GeometryPtr)->TryGetStringField(TEXT("type"), Type) || Type != TEXT("Polygon")) continue;
        const TArray<TSharedPtr<FJsonValue>>* Coordinates = nullptr;
        if (!(*GeometryPtr)->TryGetArrayField(TEXT("coordinates_cm"), Coordinates) || !Coordinates || Coordinates->Num()==0) continue;
        const TArray<TSharedPtr<FJsonValue>>* Ring = nullptr;
        if (!(*Coordinates)[0]->TryGetArray(Ring) || !Ring) continue;

        TArray<FVector> Polygon;
        for (const auto& V : *Ring)
        {
            FVector P;
            if (ReadPoint3D(V,P)) { P.Z += SurfaceOffsetCm; Polygon.Add(P); }
        }
        if (Polygon.Num()>2 && Polygon[0].Equals(Polygon.Last(),0.1f)) Polygon.Pop();
        if (Polygon.Num()<3) continue;
        TArray<int32> LocalTris;
        if (!TriangulatePolygon(Polygon, LocalTris)) continue;
        const int32 Base = Vertices.Num();
        for (const FVector& P : Polygon)
        {
            Vertices.Add(P);
            Normals.Add(FVector::UpVector);
            UV0.Add(FVector2D(P.X/800.0f,P.Y/800.0f));
            Colors.Add(FLinearColor(0.16f,0.42f,0.18f,1.0f));
            Tangents.Add(FProcMeshTangent(1,0,0));
        }
        for (int32 Idx : LocalTris) Triangles.Add(Base+Idx);
        ++LastPolygonCount;
    }

    if (Vertices.IsEmpty() || Triangles.IsEmpty()) return false;
    GreenMesh->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UV0,Colors,Tangents,bCreateCollision);
    GreenMesh->SetCanEverAffectNavigation(false);
    if (GreenAreaMaterial) GreenMesh->SetMaterial(0,GreenAreaMaterial);
    UE_LOG(LogTemp, Log, TEXT("InterVerseSG: built %d mapped green polygons in one Quest-friendly mesh section."), LastPolygonCount);
    return true;
}

bool AInterVerseGreenAreaActor::TriangulatePolygon(const TArray<FVector>& Polygon, TArray<int32>& OutTriangles)
{
    OutTriangles.Reset();
    if (Polygon.Num()<3) return false;
    TArray<int32> Remaining;
    for (int32 I=0; I<Polygon.Num(); ++I) Remaining.Add(I);
    int32 Safety=0;
    while (Remaining.Num()>3 && Safety++ < Polygon.Num()*Polygon.Num())
    {
        bool Clipped=false;
        for (int32 I=0; I<Remaining.Num(); ++I)
        {
            const int32 A=Remaining[(I-1+Remaining.Num())%Remaining.Num()];
            const int32 B=Remaining[I];
            const int32 C=Remaining[(I+1)%Remaining.Num()];
            if (Cross2D(Polygon[A],Polygon[B],Polygon[C]) <= KINDA_SMALL_NUMBER) continue;
            bool Contains=false;
            for (int32 T : Remaining)
            {
                if (T==A||T==B||T==C) continue;
                if (PointInTriangle2D(Polygon[T],Polygon[A],Polygon[B],Polygon[C])) { Contains=true; break; }
            }
            if (!Contains)
            {
                OutTriangles.Append({A,B,C});
                Remaining.RemoveAt(I);
                Clipped=true;
                break;
            }
        }
        if (!Clipped) return false;
    }
    if (Remaining.Num()==3) OutTriangles.Append({Remaining[0],Remaining[1],Remaining[2]});
    return OutTriangles.Num()>=3;
}
