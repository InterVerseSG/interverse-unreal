#include "InterVerseTerrainActor.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AInterVerseTerrainActor::AInterVerseTerrainActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
    SetRootComponent(TerrainMesh);
    TerrainMesh->SetMobility(EComponentMobility::Static);
}

void AInterVerseTerrainActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
#if WITH_EDITOR
    RebuildTerrain();
#endif
}

void AInterVerseTerrainActor::ClearTerrain()
{
    if (TerrainMesh)
    {
        TerrainMesh->ClearAllMeshSections();
    }
}

bool AInterVerseTerrainActor::RebuildTerrain()
{
    ClearTerrain();

    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TerrainJsonRelativePath)
    );

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("InterVerseSG: terrain file not found: %s"), *AbsolutePath);
        return false;
    }

    return ParseAndBuild(JsonText);
}

bool AInterVerseTerrainActor::ParseAndBuild(const FString& JsonText)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: invalid terrain JSON."));
        return false;
    }

    const TSharedPtr<FJsonObject>* GridPtr = nullptr;
    if (!Root->TryGetObjectField(TEXT("grid"), GridPtr) || !GridPtr || !GridPtr->IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: terrain JSON missing grid object."));
        return false;
    }

    const TSharedPtr<FJsonObject>& Grid = *GridPtr;
    int32 Columns = 0;
    int32 Rows = 0;
    if (!Grid->TryGetNumberField(TEXT("columns"), Columns) ||
        !Grid->TryGetNumberField(TEXT("rows"), Rows) || Columns < 2 || Rows < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: invalid terrain grid dimensions."));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* SampleRows = nullptr;
    if (!Grid->TryGetArrayField(TEXT("samples"), SampleRows) || !SampleRows || SampleRows->Num() != Rows)
    {
        UE_LOG(LogTemp, Error, TEXT("InterVerseSG: terrain sample rows do not match grid dimensions."));
        return false;
    }

    TArray<FVector> Vertices;
    Vertices.Reserve(Columns * Rows);

    for (int32 Y = 0; Y < Rows; ++Y)
    {
        const TArray<TSharedPtr<FJsonValue>>* Row = nullptr;
        if (!(*SampleRows)[Y]->TryGetArray(Row) || !Row || Row->Num() != Columns)
        {
            UE_LOG(LogTemp, Error, TEXT("InterVerseSG: malformed terrain row %d."), Y);
            return false;
        }

        for (int32 X = 0; X < Columns; ++X)
        {
            const TSharedPtr<FJsonObject>* SamplePtr = nullptr;
            if (!(*Row)[X]->TryGetObject(SamplePtr) || !SamplePtr || !SamplePtr->IsValid())
            {
                return false;
            }

            const TSharedPtr<FJsonObject>& Sample = *SamplePtr;
            double EastM = 0.0;
            double NorthM = 0.0;
            double RelativeZM = 0.0;
            Sample->TryGetNumberField(TEXT("east_m"), EastM);
            Sample->TryGetNumberField(TEXT("north_m"), NorthM);
            Sample->TryGetNumberField(TEXT("relative_z_m"), RelativeZM);

            Vertices.Add(FVector(
                static_cast<float>(EastM * 100.0),
                static_cast<float>(NorthM * 100.0),
                static_cast<float>(RelativeZM * 100.0)
            ));
        }
    }

    TArray<int32> Triangles;
    Triangles.Reserve((Columns - 1) * (Rows - 1) * 6);
    for (int32 Y = 0; Y < Rows - 1; ++Y)
    {
        for (int32 X = 0; X < Columns - 1; ++X)
        {
            const int32 A = Y * Columns + X;
            const int32 B = A + 1;
            const int32 C = A + Columns;
            const int32 D = C + 1;
            Triangles.Append({A, D, B, A, C, D});
        }
    }

    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    TerrainMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UV0,
        VertexColors,
        Tangents,
        bCreateCollision
    );

    UE_LOG(LogTemp, Log, TEXT("InterVerseSG: built terrain mesh %dx%d (%d vertices)."), Columns, Rows, Vertices.Num());
    return true;
}
