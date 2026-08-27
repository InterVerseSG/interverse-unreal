#include "InterVerseCloudClient.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "InterVerseRuntimeSettings.h"
#include "JsonObjectConverter.h"

FString UInterVerseCloudClient::JoinUrl(const FString& Base, const FString& Path) const
{
    return Base.TrimChar(TEXT('/')) + TEXT("/") + Path.TrimChar(TEXT('/'));
}

void UInterVerseCloudClient::AskAssistant(const FInterVerseAssistantRequest& RequestData)
{
    const UInterVerseRuntimeSettings* Settings = GetDefault<UInterVerseRuntimeSettings>();
    FString Body;
    if (!FJsonObjectConverter::UStructToJsonObjectString(RequestData, Body))
    {
        OnCloudError.Broadcast(TEXT("No se pudo serializar la solicitud."));
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(JoinUrl(Settings->ApiBaseUrl, TEXT("api/v1/assistant")));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Body);
    Request->SetTimeout(Settings->RequestTimeoutSeconds);

    Request->OnProcessRequestComplete().BindLambda(
        [this](FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded)
        {
            if (!bSucceeded || !Response.IsValid() || !EHttpResponseCodes::IsOk(Response->GetResponseCode()))
            {
                OnCloudError.Broadcast(TEXT("InterVerse API no respondió correctamente."));
                return;
            }

            FInterVerseCommand Command;
            if (!FJsonObjectConverter::JsonObjectStringToUStruct(Response->GetContentAsString(), &Command, 0, 0))
            {
                OnCloudError.Broadcast(TEXT("La respuesta de IA no tiene el formato esperado."));
                return;
            }

            OnAssistantCommand.Broadcast(Command);
        });

    Request->ProcessRequest();
}

void UInterVerseCloudClient::ValidateCommand(const FInterVerseCommand& Command, bool bUserConfirmed)
{
    const UInterVerseRuntimeSettings* Settings = GetDefault<UInterVerseRuntimeSettings>();

    TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("action"), Command.Action);
    Payload->SetStringField(TEXT("target"), Command.Target);
    Payload->SetStringField(TEXT("object_type"), Command.ObjectType);
    Payload->SetNumberField(TEXT("quantity"), Command.Quantity);
    Payload->SetStringField(TEXT("location"), Command.Location);
    Payload->SetBoolField(TEXT("requires_confirmation"), Command.bRequiresConfirmation);
    Payload->SetBoolField(TEXT("user_confirmed"), bUserConfirmed);

    FString Body;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
    FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(JoinUrl(Settings->BuilderBaseUrl, TEXT("api/v1/build/validate")));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Body);
    Request->SetTimeout(Settings->RequestTimeoutSeconds);

    Request->OnProcessRequestComplete().BindLambda(
        [this](FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded)
        {
            if (!bSucceeded || !Response.IsValid() || !EHttpResponseCodes::IsOk(Response->GetResponseCode()))
            {
                OnCloudError.Broadcast(TEXT("InterVerse Builder no respondió correctamente."));
                return;
            }

            FInterVerseValidatedCommand Validated;
            if (!FJsonObjectConverter::JsonObjectStringToUStruct(Response->GetContentAsString(), &Validated, 0, 0))
            {
                OnCloudError.Broadcast(TEXT("El Builder devolvió un formato no válido."));
                return;
            }

            OnCommandValidated.Broadcast(Validated);
        });

    Request->ProcessRequest();
}
