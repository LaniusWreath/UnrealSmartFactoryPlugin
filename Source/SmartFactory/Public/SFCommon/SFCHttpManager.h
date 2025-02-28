// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HttpModule.h"
#include "Http.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "SFCHttpManager.generated.h"

/**
 * Reflection 막아놨으니, 함수들 블루프린트에서 사용하고 싶으면 상속시켜 새로 클래스 만들 것
 * 
 */

class UJCMDataContainer;

DECLARE_DELEGATE_OneParam(FOnJsonObjectReceivedDelegate, const TSharedPtr<FJsonObject>);
DECLARE_DELEGATE(FOnJsonStringReceivedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDynamicRequestDelegate);

UCLASS(Blueprintable)
class SMARTFACTORY_API USFCHttpManager : public UObject
{
	GENERATED_BODY()


protected:
	FString ResultResponseString;
	TSharedPtr<FJsonObject> ParsedJsonData;

	FString TempResultResponseString;

	FTimerHandle CancleTimerHandle;

	int32 TimeoutCount=0;
	int32 MaxTimeoutCount = 2;

	// Current Request Pointer
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;

protected:

	// Main Request Function

	// Request work delegate binding functions
	virtual void OnResponseReceivedWithString(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful); // with string
	virtual void OnResponseReceivedWithPtr(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful); // with objectptr

	// If you want to adjust custom parsing function, override this
	virtual void CallCustomParseFucntion(TSharedPtr<FJsonObject> OriginJsonObject);

	// Json Object -> String to Json Object
	virtual TSharedPtr<FJsonObject> ParseRequestBody(TSharedPtr<FJsonObject> RequestBody);

	// Json Object -> TMap<FString, FString>
	static TMap<FString, FString> ParseJsonObjToMap(const TSharedPtr<FJsonObject> OriginJsonObject);

	

public:
	// Delegate for Alarming Request Done, Data Ready
	FOnJsonObjectReceivedDelegate OnParsedJsonObjectPtrReady;
	FOnJsonStringReceivedDelegate OnRequestedJsonStringReady;

	// Blueprint Callable Delegate 
	UPROPERTY(BlueprintAssignable, Category = "SFC|HTTP")
	FOnDynamicRequestDelegate OnDynamicRequestingEvent;

	UFUNCTION(BlueprintCallable, Category = "SFC|HTTP")
	static USFCHttpManager* CreateHttpManagerInstance(UObject* Outer, TSubclassOf<USFCHttpManager> ManagerClass);

	// ---------------------------------------------- Get ---------------------------------------------

	// RequestFunction only URL
	UFUNCTION(BlueprintCallable, Category = "SFC|HTTP")
	virtual void MakeGetRequest(const FString& Url, const bool GetResultWithFString = true);

	// Reqeust Function URL with Header
	UFUNCTION(BlueprintCallable, Category = "SFC|HTTP")
	virtual void MakeGetRequestWithHeader(const FString& Url, const TMap<FString, FString>& Headers,
		const TMap<FString, FString>& Parameters, const bool GetResultWithFString = true);

	// Return Serialized JsonString member
	UFUNCTION(BlueprintPure, Category = "SFC|HTTP")
	const FString& GetResultResponseString()
	{
		return ResultResponseString;
	}

	UFUNCTION(BlueprintPure, Category = "SFC|HTTP")
	const FString& GetTempResultResponseString()
	{
		return TempResultResponseString;
	}


	// -------------------------------------------- Post ---------------------------------------------

	void SendPostRequest(const FString& Url, const TMap<FString, FString>& Headers, const FString& PostData, bool GetResultWithFString);

	FString CreateJsonString(const TMap<FString, FString>& DataMap);

	UFUNCTION(BlueprintCallable, Category = "SFC|HTTP", meta = (AdvancedDisplay = "bTimerOn,TimerInterval"))
	void MakePostRequest(const FString& URL, const TMap<FString, FString>& InHeaders, const TMap<FString, FString> InData);


	FString ConvertTMapToJson(const TMap<FString, FString>& DataMap);


	// Cancle
	void CancelHttpRequest();

public:

	// Return JsonObjectPtr member
	const TSharedPtr<FJsonObject> GetJsonObject(){
		return ParsedJsonData;
	};

};

