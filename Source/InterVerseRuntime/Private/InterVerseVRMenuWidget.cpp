#include "InterVerseVRMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "InterVerseCloudClient.h"
#include "InterVerseXRPawn.h"
#include "InterVerseNavigationComponent.h"

namespace
{
UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 Size)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Text));
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = Size;
    Label->SetFont(Font);
    return Label;
}

UButton* MakeButton(UWidgetTree* Tree, const FString& Text)
{
    UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
    UTextBlock* Label = MakeText(Tree, Text, 30);
    Button->AddChild(Label);
    return Button;
}
}

void UInterVerseVRMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    WidgetTree->RootWidget = Root;

    UTextBlock* Title = MakeText(WidgetTree, TEXT("InterVerseSG · Destinos"), 38);
    Root->AddChildToVerticalBox(Title);

    UTextBlock* Subtitle = MakeText(WidgetTree, TEXT("Selecciona un destino para iniciar la guía VR."), 23);
    Root->AddChildToVerticalBox(Subtitle);

    UButton* Marquis = MakeButton(WidgetTree, TEXT("Marquis Science Hall"));
    Marquis->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::GuideMarquis);
    Root->AddChildToVerticalBox(Marquis);

    UButton* Cai = MakeButton(WidgetTree, TEXT("CAI"));
    Cai->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::GuideCAI);
    Root->AddChildToVerticalBox(Cai);

    UButton* StudentCenter = MakeButton(WidgetTree, TEXT("Centro de Estudiantes"));
    StudentCenter->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::GuideStudentCenter);
    Root->AddChildToVerticalBox(StudentCenter);

    UButton* Graduate = MakeButton(WidgetTree, TEXT("Escuela de Estudios Graduados e Investigación"));
    Graduate->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::GuideGraduateSchool);
    Root->AddChildToVerticalBox(Graduate);

    UButton* AIGraduate = MakeButton(WidgetTree, TEXT("IA: Guíame a Escuela Graduada"));
    AIGraduate->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::AskGraduateSchoolAI);
    Root->AddChildToVerticalBox(AIGraduate);

    StatusText = MakeText(WidgetTree, TEXT("Gemini + Builder configurados · Guía lista"), 21);
    Root->AddChildToVerticalBox(StatusText);

    UButton* Close = MakeButton(WidgetTree, TEXT("Cerrar menú"));
    Close->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::CloseMenu);
    Root->AddChildToVerticalBox(Close);

    if (AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn()))
    {
        if (Pawn->CloudClient)
        {
            Pawn->CloudClient->OnCommandValidated.AddDynamic(this, &UInterVerseVRMenuWidget::HandleValidatedCommand);
            Pawn->CloudClient->OnCloudError.AddDynamic(this, &UInterVerseVRMenuWidget::HandleCloudError);
        }
    }
}

void UInterVerseVRMenuWidget::SetStatusText(const FString& Message)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Message));
    }
}

void UInterVerseVRMenuWidget::StartGuidance(const FString& NavigationAnchor, const FString& DisplayName)
{
    AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn());
    if (!Pawn || !Pawn->Navigation)
    {
        SetStatusText(TEXT("Navegación VR no disponible."));
        return;
    }

    if (Pawn->Navigation->StartGuidanceToAnchor(NavigationAnchor))
    {
        SetStatusText(FString::Printf(TEXT("Guía activa → %s"), *DisplayName));
    }
    else
    {
        SetStatusText(FString::Printf(TEXT("No se encontró %s en el nivel."), *DisplayName));
    }
}

void UInterVerseVRMenuWidget::GuideMarquis()
{
    StartGuidance(TEXT("NAV_MarquisScienceHall"), TEXT("Marquis Science Hall"));
}

void UInterVerseVRMenuWidget::GuideCAI()
{
    StartGuidance(TEXT("NAV_CAI"), TEXT("CAI"));
}

void UInterVerseVRMenuWidget::GuideStudentCenter()
{
    StartGuidance(TEXT("NAV_CentroEstudiantes"), TEXT("Centro de Estudiantes"));
}

void UInterVerseVRMenuWidget::GuideGraduateSchool()
{
    StartGuidance(TEXT("NAV_EscuelaGraduada"), TEXT("Escuela Graduada"));
}

void UInterVerseVRMenuWidget::AskGraduateSchoolAI()
{
    AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn());
    if (!Pawn || !Pawn->CloudClient)
    {
        SetStatusText(TEXT("Cliente Gemini/Builder no disponible."));
        return;
    }

    FInterVerseAssistantRequest Request;
    Request.Message = TEXT("Guíame a la Escuela Graduada");
    Request.Context = TEXT("Meta Quest VR campus navigation menu");
    Request.SessionId = TEXT("quest-vr-menu");
    SetStatusText(TEXT("Consultando Gemini y validando con Builder..."));
    Pawn->CloudClient->AskAssistant(Request);
}

void UInterVerseVRMenuWidget::HandleValidatedCommand(const FInterVerseValidatedCommand& Command)
{
    if (!Command.bAccepted)
    {
        SetStatusText(Command.Message.IsEmpty() ? TEXT("Builder rechazó la instrucción.") : Command.Message);
        return;
    }

    if (Command.Action.Equals(TEXT("navigate"), ESearchCase::IgnoreCase))
    {
        SetStatusText(FString::Printf(TEXT("IA validada → %s · guía activada"), *Command.NavigationAnchor));
    }
    else
    {
        SetStatusText(FString::Printf(TEXT("IA validada: %s"), *Command.Action));
    }
}

void UInterVerseVRMenuWidget::HandleCloudError(const FString& Message)
{
    SetStatusText(FString::Printf(TEXT("Error cloud: %s"), *Message));
}

void UInterVerseVRMenuWidget::CloseMenu()
{
    if (AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn()))
    {
        Pawn->SetVRMenuVisible(false);
    }
}
