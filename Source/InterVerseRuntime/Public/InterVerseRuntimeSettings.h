#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InterVerseRuntimeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="InterVerse Runtime"))
class UInterVerseRuntimeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Cloud")
    FString ApiBaseUrl = TEXT("https://interverse-api.onrender.com");

    UPROPERTY(Config, EditAnywhere, Category="Cloud")
    FString BuilderBaseUrl = TEXT("https://interverse-builder.onrender.com");

    UPROPERTY(Config, EditAnywhere, Category="Cloud", meta=(ClampMin="1.0", ClampMax="60.0"))
    float RequestTimeoutSeconds = 20.0f;
};
