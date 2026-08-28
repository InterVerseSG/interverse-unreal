#include "InterVerseVRMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Dom/JsonObject.h"
#include "InterVerseCloudClient.h"
#include "InterVerseNavigationComponent.h"
#include "InterVerseXRPawn.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const FLinearColor InterGreen(0.0f, 0.482f, 0.373f, 1.0f);
const FLinearColor InterYellow(0.996f, 0.82f, 0.255f, 1.0f);
const FLinearColor InterDark(0.04f, 0.10f, 0.08f, 1.0f);

UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& Color = FLinearColor::White)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Text));
    Label->SetColorAndOpacity(FSlateColor(Color));
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = Size;
    Label->SetFont(Font);
    return Label;
}

UButton* MakeButton(UWidgetTree* Tree, const FString& Text, const FLinearColor& Background)
{
    UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
    Button->SetBackgroundColor(Background);
    UTextBlock* Label = MakeText(Tree, Text, 27, InterDark);
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

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(FLinearColor(0.01f, 0.08f, 0.06f, 0.96f));
    WidgetTree->RootWidget = Panel;

    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Panel->AddChild(Root);

    Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("InterVerseSG · Navegación VR"), 38, InterYellow));
    Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Selecciona una categoría y un destino del Recinto de San Germán."), 22));

    LoadDestinations();

    Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Categoría"), 22, InterYellow));
    CategoryCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
    CategoryCombo->OnSelectionChanged.AddDynamic(this, &UInterVerseVRMenuWidget::HandleCategoryChanged);
    Root->AddChildToVerticalBox(CategoryCombo);

    Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Buscar destino"), 22, InterYellow));
    SearchBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    SearchBox->SetHintText(FText::FromString(TEXT("Ej. CAI, Phraner, Escuela Graduada")));
    SearchBox->OnTextChanged.AddDynamic(this, &UInterVerseVRMenuWidget::HandleSearchChanged);
    Root->AddChildToVerticalBox(SearchBox);

    Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Destino"), 22, InterYellow));
    DestinationCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
    Root->AddChildToVerticalBox(DestinationCombo);

    UButton* Guide = MakeButton(WidgetTree, TEXT("Iniciar guía al destino"), InterYellow);
    Guide->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::GuideSelectedDestination);
    Root->AddChildToVerticalBox(Guide);

    UButton* Stop = MakeButton(WidgetTree, TEXT("Detener guía activa"), FLinearColor(0.82f, 0.82f, 0.82f, 1.0f));
    Stop->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::StopActiveGuidance);
    Root->AddChildToVerticalBox(Stop);

    UButton* AIGraduate = MakeButton(WidgetTree, TEXT("IA: Guíame a Escuela Graduada"), InterGreen);
    AIGraduate->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::AskGraduateSchoolAI);
    Root->AddChildToVerticalBox(AIGraduate);

    StatusText = MakeText(WidgetTree, TEXT("Gemini + Builder configurados · Guía lista"), 20, FLinearColor::White);
    Root->AddChildToVerticalBox(StatusText);

    UButton* Close = MakeButton(WidgetTree, TEXT("Cerrar menú"), InterYellow);
    Close->OnClicked.AddDynamic(this, &UInterVerseVRMenuWidget::CloseMenu);
    Root->AddChildToVerticalBox(Close);

    RebuildCategoryOptions();
    RebuildDestinationOptions();

    if (AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn()))
    {
        if (Pawn->CloudClient)
        {
            Pawn->CloudClient->OnCommandValidated.AddDynamic(this, &UInterVerseVRMenuWidget::HandleValidatedCommand);
            Pawn->CloudClient->OnCloudError.AddDynamic(this, &UInterVerseVRMenuWidget::HandleCloudError);
        }
    }
}

bool UInterVerseVRMenuWidget::LoadDestinations()
{
    Destinations.Reset();
    const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/InterVerseCampusAnchors.json"));
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *Path))
    {
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Anchors = nullptr;
    if (!Root->TryGetArrayField(TEXT("anchors"), Anchors) || !Anchors)
    {
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *Anchors)
    {
        const TSharedPtr<FJsonObject>* Entry = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Entry) || !Entry || !Entry->IsValid())
        {
            continue;
        }

        FString Id;
        FString Nav;
        FString Display;
        (*Entry)->TryGetStringField(TEXT("id"), Id);
        (*Entry)->TryGetStringField(TEXT("navigation_anchor"), Nav);
        (*Entry)->TryGetStringField(TEXT("display_name"), Display);
        if (Id.IsEmpty() || Nav.IsEmpty())
        {
            continue;
        }

        FInterVerseVRMenuDestination Destination;
        Destination.Id = Id;
        Destination.DisplayName = DisplayNameForId(Id, Display);
        Destination.NavigationAnchor = Nav;
        Destination.Category = CategoryForDestination(Id, Destination.DisplayName);
        Destinations.Add(Destination);
    }

    Destinations.Sort([](const FInterVerseVRMenuDestination& A, const FInterVerseVRMenuDestination& B)
    {
        return A.DisplayName < B.DisplayName;
    });
    return Destinations.Num() > 0;
}

FString UInterVerseVRMenuWidget::DisplayNameForId(const FString& Id, const FString& ExplicitDisplayName) const
{
    if (!ExplicitDisplayName.IsEmpty()) return ExplicitDisplayName;
    if (Id == TEXT("JuanCancioOrtizCAI")) return TEXT("Centro de Acceso a la Información (CAI)");
    if (Id == TEXT("CentroEstudiantesJamesABeverly")) return TEXT("Centro de Estudiantes James A. Beverly");
    if (Id == TEXT("PolideportivoLuisFSambolin")) return TEXT("Polideportivo Luis F. Sambolín");
    if (Id == TEXT("PistaLuisFSambolinAlsina")) return TEXT("Pista Luis F. Sambolín Alsina");
    if (Id == TEXT("CapillaPaulAWolfeMemorial")) return TEXT("Capilla Paul A. Wolfe Memorial");
    if (Id == TEXT("CampusSchoolAnexo2")) return TEXT("Campus School / Anexo 2");
    if (Id == TEXT("CampusMain")) return TEXT("Área principal del campus");
    return Id;
}

FString UInterVerseVRMenuWidget::CategoryForDestination(const FString& Id, const FString& DisplayName) const
{
    const FString Search = (Id + TEXT(" ") + DisplayName).ToLower();
    if (Search.Contains(TEXT("polideportivo")) || Search.Contains(TEXT("pista"))) return TEXT("Deportes");
    if (Search.Contains(TEXT("capilla")) || Search.Contains(TEXT("cottage"))) return TEXT("Vida universitaria");
    if (Search.Contains(TEXT("centro de estudiantes"))) return TEXT("Servicios estudiantiles");
    if (Search.Contains(TEXT("cai")) || Search.Contains(TEXT("información"))) return TEXT("Biblioteca y recursos");
    if (Search.Contains(TEXT("escuela de estudios graduados")) || Search.Contains(TEXT("escuelagraduada"))) return TEXT("Estudios graduados");
    if (Search.Contains(TEXT("campus school")) || Search.Contains(TEXT("anexo"))) return TEXT("Escuela y anexos");
    if (Search.Contains(TEXT("hall")) || Search.Contains(TEXT("carlos")) || Search.Contains(TEXT("leopoldo")) || Search.Contains(TEXT("eusebio"))) return TEXT("Edificios académicos");
    return TEXT("Otros");
}

void UInterVerseVRMenuWidget::RebuildCategoryOptions()
{
    if (!CategoryCombo) return;
    CategoryCombo->ClearOptions();
    CategoryCombo->AddOption(TEXT("Todos"));
    TSet<FString> Categories;
    for (const FInterVerseVRMenuDestination& D : Destinations) Categories.Add(D.Category);
    TArray<FString> Sorted = Categories.Array();
    Sorted.Sort();
    for (const FString& Category : Sorted) CategoryCombo->AddOption(Category);
    CategoryCombo->SetSelectedOption(ActiveCategory);
}

void UInterVerseVRMenuWidget::RebuildDestinationOptions()
{
    if (!DestinationCombo) return;
    DestinationCombo->ClearOptions();
    const FString Needle = SearchFilter.ToLower();
    for (const FInterVerseVRMenuDestination& D : Destinations)
    {
        if (ActiveCategory != TEXT("Todos") && D.Category != ActiveCategory) continue;
        const FString Haystack = (D.DisplayName + TEXT(" ") + D.NavigationAnchor).ToLower();
        if (!Needle.IsEmpty() && !Haystack.Contains(Needle)) continue;
        DestinationCombo->AddOption(D.DisplayName);
    }
    if (DestinationCombo->GetOptionCount() > 0)
    {
        DestinationCombo->SetSelectedIndex(0);
        SetStatusText(FString::Printf(TEXT("%d destinos disponibles"), DestinationCombo->GetOptionCount()));
    }
    else
    {
        SetStatusText(TEXT("No hay destinos que coincidan con el filtro."));
    }
}

const FInterVerseVRMenuDestination* UInterVerseVRMenuWidget::FindSelectedDestination() const
{
    if (!DestinationCombo) return nullptr;
    const FString Selected = DestinationCombo->GetSelectedOption();
    for (const FInterVerseVRMenuDestination& D : Destinations)
    {
        if (D.DisplayName == Selected) return &D;
    }
    return nullptr;
}

void UInterVerseVRMenuWidget::SetStatusText(const FString& Message)
{
    if (StatusText) StatusText->SetText(FText::FromString(Message));
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
        SetStatusText(FString::Printf(TEXT("Guía activa → %s"), *DisplayName));
    else
        SetStatusText(FString::Printf(TEXT("No se encontró %s en el nivel."), *DisplayName));
}

void UInterVerseVRMenuWidget::HandleCategoryChanged(FString SelectedItem, ESelectInfo::Type)
{
    ActiveCategory = SelectedItem.IsEmpty() ? TEXT("Todos") : SelectedItem;
    RebuildDestinationOptions();
}

void UInterVerseVRMenuWidget::HandleSearchChanged(const FText& Text)
{
    SearchFilter = Text.ToString().TrimStartAndEnd();
    RebuildDestinationOptions();
}

void UInterVerseVRMenuWidget::GuideSelectedDestination()
{
    const FInterVerseVRMenuDestination* D = FindSelectedDestination();
    if (!D)
    {
        SetStatusText(TEXT("Selecciona un destino válido."));
        return;
    }
    StartGuidance(D->NavigationAnchor, D->DisplayName);
}

void UInterVerseVRMenuWidget::StopActiveGuidance()
{
    if (AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn()))
    {
        if (Pawn->Navigation)
        {
            Pawn->Navigation->StopGuidance();
            SetStatusText(TEXT("Guía detenida."));
            return;
        }
    }
    SetStatusText(TEXT("Navegación VR no disponible."));
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
        SetStatusText(FString::Printf(TEXT("IA validada → %s · guía activada"), *Command.NavigationAnchor));
    else
        SetStatusText(FString::Printf(TEXT("IA validada: %s"), *Command.Action));
}

void UInterVerseVRMenuWidget::HandleCloudError(const FString& Message)
{
    SetStatusText(FString::Printf(TEXT("Error cloud: %s"), *Message));
}

void UInterVerseVRMenuWidget::CloseMenu()
{
    if (AInterVerseXRPawn* Pawn = Cast<AInterVerseXRPawn>(GetOwningPlayerPawn())) Pawn->SetVRMenuVisible(false);
}
