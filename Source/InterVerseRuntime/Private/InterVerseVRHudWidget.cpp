#include "InterVerseVRHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"
#include "InterVerseCloudClient.h"
#include "InterVerseNavigationComponent.h"
#include "InterVerseXRPawn.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
UTextBlock* IVMakeHudText(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& Color)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Text));
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = Size;
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(Color));
    return Label;
}
}

void UInterVerseVRHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRuntimeWidget();

    if (AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn()))
    {
        if (Pawn->CloudClient)
        {
            Pawn->CloudClient->OnCommandValidated.AddDynamic(this, &UInterVerseVRHudWidget::HandleValidatedCommand);
            Pawn->CloudClient->OnCloudError.AddDynamic(this, &UInterVerseVRHudWidget::HandleCloudError);
        }
    }
}

void UInterVerseVRHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshNavigation();
}

void UInterVerseVRHudWidget::BuildRuntimeWidget()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    RootBorder->SetBrushColor(FLinearColor(0.0f, 0.31f, 0.24f, 0.94f));
    RootBorder->SetPadding(FMargin(18.0f, 12.0f));
    WidgetTree->RootWidget = RootBorder;

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    RootBorder->SetContent(Box);

    DestinationText = IVMakeHudText(WidgetTree, TEXT("InterVerseSG · Navegación lista"), 28, FLinearColor::White);
    Box->AddChildToVerticalBox(DestinationText);

    DistanceText = IVMakeHudText(WidgetTree, TEXT("Selecciona un destino desde el menú VR."), 24, FLinearColor(0.996f, 0.82f, 0.25f, 1.0f));
    Box->AddChildToVerticalBox(DistanceText);

    DirectionTextBlock = IVMakeHudText(WidgetTree, TEXT(""), 22, FLinearColor::White);
    Box->AddChildToVerticalBox(DirectionTextBlock);

    CloudText = IVMakeHudText(WidgetTree, TEXT("Gemini + Builder · listos"), 18, FLinearColor(0.85f, 0.92f, 0.89f, 1.0f));
    Box->AddChildToVerticalBox(CloudText);
}

void UInterVerseVRHudWidget::RefreshNavigation()
{
    AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn());
    if (!Pawn || !Pawn->Navigation || !DestinationText || !DistanceText || !DirectionTextBlock)
    {
        return;
    }

    if (!Pawn->Navigation->IsGuidanceActive())
    {
        LastAnchor.Reset();
        bArrivalShown = false;
        DestinationText->SetText(FText::FromString(TEXT("InterVerseSG · Navegación lista")));
        DistanceText->SetText(FText::FromString(TEXT("Selecciona un destino desde el menú VR.")));
        DirectionTextBlock->SetText(FText::GetEmpty());
        return;
    }

    const FString Anchor = Pawn->Navigation->GetGuidanceAnchor();
    const float DistanceCm = Pawn->Navigation->GetGuidanceDistanceCm();
    const FString Friendly = FriendlyNameForAnchor(Anchor);

    if (LastAnchor != Anchor)
    {
        LastAnchor = Anchor;
        bArrivalShown = false;
    }

    DestinationText->SetText(FText::FromString(FString::Printf(TEXT("Destino · %s"), *Friendly)));

    if (DistanceCm <= ArrivalThresholdCm)
    {
        DistanceText->SetText(FText::FromString(TEXT("Llegaste al destino")));
        DirectionTextBlock->SetText(FText::FromString(TEXT("Puedes detener la guía desde el menú VR.")));
        bArrivalShown = true;
        return;
    }

    const float DistanceMeters = DistanceCm / 100.0f;
    DistanceText->SetText(FText::FromString(FString::Printf(TEXT("%.0f m restantes"), DistanceMeters)));
    DirectionTextBlock->SetText(FText::FromString(DirectionText(Pawn->Navigation->GetGuidanceDirection())));
}

FString UInterVerseVRHudWidget::FriendlyNameForAnchor(const FString& NavigationAnchor) const
{
    const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/InterVerseCampusAnchors.json"));
    FString JsonText;
    if (FFileHelper::LoadFileToString(JsonText, *Path))
    {
        TSharedPtr<FJsonObject> Root;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
        if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* Anchors = nullptr;
            if (Root->TryGetArrayField(TEXT("anchors"), Anchors) && Anchors)
            {
                for (const TSharedPtr<FJsonValue>& Value : *Anchors)
                {
                    const TSharedPtr<FJsonObject>* Object = nullptr;
                    if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
                    {
                        continue;
                    }
                    FString Nav;
                    (*Object)->TryGetStringField(TEXT("navigation_anchor"), Nav);
                    if (Nav != NavigationAnchor)
                    {
                        continue;
                    }
                    FString Display;
                    if ((*Object)->TryGetStringField(TEXT("display_name"), Display) && !Display.IsEmpty())
                    {
                        return Display;
                    }
                    FString Id;
                    if ((*Object)->TryGetStringField(TEXT("id"), Id) && !Id.IsEmpty())
                    {
                        return Id;
                    }
                }
            }
        }
    }

    FString Fallback = NavigationAnchor;
    Fallback.RemoveFromStart(TEXT("NAV_"));
    return Fallback;
}

FString UInterVerseVRHudWidget::DirectionText(const FVector& WorldDirection) const
{
    const APlayerCameraManager* CameraManager = GetOwningPlayer() ? GetOwningPlayer()->PlayerCameraManager : nullptr;
    if (!CameraManager)
    {
        return TEXT("Sigue la flecha de navegación");
    }

    FVector FlatDirection = WorldDirection;
    FlatDirection.Z = 0.0f;
    if (!FlatDirection.Normalize())
    {
        return TEXT("Sigue la flecha de navegación");
    }

    const float TargetYaw = FlatDirection.Rotation().Yaw;
    const float ViewYaw = CameraManager->GetCameraRotation().Yaw;
    const float Delta = FMath::FindDeltaAngleDegrees(ViewYaw, TargetYaw);

    if (FMath::Abs(Delta) <= 20.0f)
    {
        return TEXT("Continúa al frente");
    }
    if (Delta > 20.0f && Delta < 150.0f)
    {
        return TEXT("Gira hacia la derecha");
    }
    if (Delta < -20.0f && Delta > -150.0f)
    {
        return TEXT("Gira hacia la izquierda");
    }
    return TEXT("El destino está detrás de ti");
}

void UInterVerseVRHudWidget::SetCloudStatus(const FString& Message, bool bError)
{
    if (!CloudText)
    {
        return;
    }
    CloudText->SetText(FText::FromString(Message));
    CloudText->SetColorAndOpacity(FSlateColor(bError ? FLinearColor(1.0f, 0.48f, 0.38f, 1.0f) : FLinearColor(0.85f, 0.92f, 0.89f, 1.0f)));
}

void UInterVerseVRHudWidget::HandleValidatedCommand(const FInterVerseValidatedCommand& Command)
{
    if (Command.bAccepted)
    {
        SetCloudStatus(TEXT("Gemini + Builder · comando validado"), false);
    }
    else
    {
        SetCloudStatus(TEXT("Builder rechazó la instrucción"), true);
    }
}

void UInterVerseVRHudWidget::HandleCloudError(const FString& Message)
{
    SetCloudStatus(FString::Printf(TEXT("Cloud · %s"), *Message), true);
}
