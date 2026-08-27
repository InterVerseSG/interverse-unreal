#pragma once

#include "CoreMinimal.h"
#include "InterVerseTypes.generated.h"

USTRUCT(BlueprintType)
struct FInterVerseAssistantRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Context;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SessionId;
};

USTRUCT(BlueprintType)
struct FInterVerseCommand
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString Action;

    UPROPERTY(BlueprintReadOnly)
    FString Response;

    UPROPERTY(BlueprintReadOnly)
    FString Target;

    UPROPERTY(BlueprintReadOnly)
    FString ObjectType;

    UPROPERTY(BlueprintReadOnly)
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly)
    FString Location;

    UPROPERTY(BlueprintReadOnly)
    bool bRequiresConfirmation = false;
};

USTRUCT(BlueprintType)
struct FInterVerseValidatedCommand
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bAccepted = false;

    UPROPERTY(BlueprintReadOnly)
    FString Action;

    UPROPERTY(BlueprintReadOnly)
    FString BlueprintClass;

    UPROPERTY(BlueprintReadOnly)
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly)
    FString Location;

    UPROPERTY(BlueprintReadOnly)
    FString Target;

    UPROPERTY(BlueprintReadOnly)
    bool bRequiresConfirmation = false;

    UPROPERTY(BlueprintReadOnly)
    FString Reason;
};
