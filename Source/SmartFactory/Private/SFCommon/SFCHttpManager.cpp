// Fill out your copyright notice in the Description page of Project Settings.


#include "SFCommon/SFCHttpManager.h"
#include "SFCommon/SFCLog.h"
#include "SFCommon/SFCDataManageUtilities.h"

USFCHttpManager* USFCHttpManager::CreateHttpManagerInstance(UObject* Outer, TSubclassOf<USFCHttpManager> ManagerClass)
{
	// 클래스 지정 안한 경우, CPP기반 StaticClass
	if (!ManagerClass)
	{
		return NewObject<USFCHttpManager>(Outer, USFCHttpManager::StaticClass());
	}
	// 클래스 지정 한 경우, BP 기반 상속 클래스
	return NewObject<USFCHttpManager>(Outer, ManagerClass);
}

// 헤더 없는 request 함수
void USFCHttpManager::MakeGetRequest(const FString& Url, const bool GetResultWithFString)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http) return;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));

	// 응답 함수 델리게이트 바인딩.
	if (GetResultWithFString)
	{
		Request->OnProcessRequestComplete().BindUObject(this, &USFCHttpManager::OnResponseReceivedWithString);
	}
	else
	{
		Request->OnProcessRequestComplete().BindUObject(this, &USFCHttpManager::OnResponseReceivedWithPtr);
	}

	// 요청 실행.
	Request->ProcessRequest();
}

// 헤더 포함해서 request
void USFCHttpManager::MakeGetRequestWithHeader(const FString& Url, const TMap<FString, FString>& Headers, 
	const TMap<FString, FString>& Parameters, const bool GetResultWithFString)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http) return;

	TimeoutCount++;

	if (TimeoutCount > MaxTimeoutCount)
	{
		UE_LOG(SFClog, Error, TEXT("GetRequest Queue is full"));
		return;
	}

	// URL에 파라미터 추가
	FString FinalUrl = Url;
	if (Parameters.Num() > 0)
	{
		FinalUrl += TEXT("?");
		for (const TPair<FString, FString>& Param : Parameters)
		{
			FinalUrl += FString::Printf(TEXT("%s=%s&"), *Param.Key, *Param.Value);
		}
		// 마지막 '&' 제거
		FinalUrl.RemoveFromEnd(TEXT("&"));
	}

	// 요청 생성
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	Request->SetURL(FinalUrl);
	Request->SetVerb(TEXT("GET"));

	// 헤더 설정
	for (const TPair<FString, FString>& Header : Headers)
	{
		Request->SetHeader(Header.Key, Header.Value);
	}

	// 응답 함수 델리게이트 바인딩
	if (GetResultWithFString)
	{
		Request->OnProcessRequestComplete().BindUObject(this, &USFCHttpManager::OnResponseReceivedWithString);
	}
	else
	{
		Request->OnProcessRequestComplete().BindUObject(this, &USFCHttpManager::OnResponseReceivedWithPtr);
	}

	// 요청 실행
	Request->ProcessRequest();
}

// ---------------------------------------------------- POST --------------------------------------------------------

void USFCHttpManager::SendPostRequest(const FString& Url, const TMap<FString, FString>& Headers, const FString& PostData, bool GetResultWithFString)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http) return;

	// 요청 생성
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));  // POST 요청 설정
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));  // JSON 데이터 전송

	// Override My header
	for (const TPair<FString, FString>& Header : Headers)
	{
		Request->SetHeader(Header.Key, Header.Value);
	}

	// Set Post Data
	Request->SetContentAsString(PostData);

	// Bind Response Delegates
	if (GetResultWithFString)
	{
		Request->OnProcessRequestComplete().BindUObject(this, &USFCHttpManager::OnResponseReceivedWithString);
	}
	else
	{
		Request->OnProcessRequestComplete().BindUObject(this, &USFCHttpManager::OnResponseReceivedWithPtr);
	}

	// Do Request
	Request->ProcessRequest();

	// Save Current Request
	CurrentRequest = Request;

	// Cancle if timer end
	GetWorld()->GetTimerManager().SetTimer(CancleTimerHandle, this, &USFCHttpManager::CancelHttpRequest, 5.0f, false);
}


void USFCHttpManager::MakePostRequest(const FString& URL, const TMap<FString, FString>& InHeaders, const TMap<FString, FString> InData)
{
	TimeoutCount++;

	if (TimeoutCount > MaxTimeoutCount)
	{
		UE_LOG(SFClog, Error, TEXT("PostRequest Queue is full"));
		return;
	}

	FString JsonData = ConvertTMapToJson(InData);

	SendPostRequest(URL, InHeaders, JsonData, true);
}

// Cancle if request response delayed
void USFCHttpManager::CancelHttpRequest()
{
	if (CurrentRequest.IsValid() && CurrentRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		UE_LOG(SFClog, Warning, TEXT("Request Timeout: Cancelling Request"));
		CurrentRequest->CancelRequest();
	}
}


FString USFCHttpManager::ConvertTMapToJson(const TMap<FString, FString>& DataMap)
{

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	for (const TPair<FString, FString>& Pair : DataMap)
	{
		FString Key = Pair.Key;
		FString Value = Pair.Value;

		if (Value.Equals("true", ESearchCase::IgnoreCase))
		{
			JsonObject->SetBoolField(Key, true);
		}
		else if (Value.Equals("false", ESearchCase::IgnoreCase))
		{
			JsonObject->SetBoolField(Key, false);
		}

		else if (Value.IsNumeric())
		{
			JsonObject->SetNumberField(Key, FCString::Atof(*Value));
		}

		else
		{
			JsonObject->SetStringField(Key, Value);
		}
	}

	// Json to String
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		return OutputString; 
	}

	UE_LOG(SFClog, Error, TEXT("Converted failed"));
	return TEXT("{}");
}


//------------------------------------------------------------------------------------------------------------//

FString USFCHttpManager::CreateJsonString(const TMap<FString, FString>& DataMap)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	for (const TPair<FString, FString>& Pair : DataMap)
	{
		JsonObject->SetStringField(Pair.Key, Pair.Value);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	UE_LOG(SFClog, Log, TEXT("CreateJsonString : %s"), *OutputString);
	return OutputString;
}


// Request 응답 바인딩 함수 : 문자열로 요청했을 경우
void USFCHttpManager::OnResponseReceivedWithString(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	// 응답 데이터 확인.
	if (bWasSuccessful && Response.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CancleTimerHandle);

		TimeoutCount = 0;

		// 결과는 HttpHandler 인스턴스의 ResultResponseString에 저장.
		TempResultResponseString = Response->GetContentAsString();

		// 전체 결과 중 data 필드에 해당하는 값만 떼어내 저장
		//ResultResponseString = USFCDataManageUtilities::ExtractDataFieldFromJsonString(TempResultResponseString);

		// 델리게이트에 바인딩된 함수가 있을때만 execute() : cpp 전용
		if (OnRequestedJsonStringReady.IsBound())
		{
			OnRequestedJsonStringReady.Execute();
		}
		// 다이나믹 델리게이트는 조건 없이 
		OnDynamicRequestingEvent.Broadcast();
	}
	else
	{
		UE_LOG(SFClog, Error, TEXT("%s : HTTP Request failed."), *this->GetName());
		TimeoutCount++;
	}
}

// Request 응답 바인딩 함수 : 객체 포인터로 요청했을 경우 : 블루프린트에 리플렉션 되지 않으므로, cpp에서 추가 작업 요구됨.
void USFCHttpManager::OnResponseReceivedWithPtr(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CancleTimerHandle);

		TimeoutCount = 0;

		// 응답 데이터 확인
		ResultResponseString = Response->GetContentAsString();

		// JSON 파싱
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResultResponseString);
		if (FJsonSerializer::Deserialize(Reader, ParsedJsonData))
		{
			// 파싱 실행 함수 호출로 대체 할 수 있지만 지금 형태가 default
			if (OnParsedJsonObjectPtrReady.IsBound())
			{
				OnParsedJsonObjectPtrReady.Execute(ParsedJsonData);
			}

			// 블루프린트용 다이나믹 멀티캐스트 델리게이트
			OnDynamicRequestingEvent.Broadcast();
		}
		else
		{
			UE_LOG(SFClog, Error, TEXT("%s : Failed to parse JSON."), *this->GetName());
		}
	}
	else
	{
		UE_LOG(SFClog, Error, TEXT("%s : HTTP Request failed."), *this->GetName());
	}
}

// 커스텀 파싱 함수 호출 함수.
void USFCHttpManager::CallCustomParseFucntion(TSharedPtr<FJsonObject> OriginJsonObject)
{
	ParsedJsonData = ParseRequestBody(OriginJsonObject);
	if (ParsedJsonData)
	{
		if (OnParsedJsonObjectPtrReady.IsBound())
		{
			OnParsedJsonObjectPtrReady.Execute(ParsedJsonData);
		}
		OnDynamicRequestingEvent.Broadcast();
	}
}

// 기본 파싱 함수: 커스텀 파싱은 상속하여 처음부터 다시 작성할 것.
TSharedPtr<FJsonObject> USFCHttpManager::ParseRequestBody(TSharedPtr<FJsonObject> RequestBody)
{
	const TSharedPtr<FJsonObject> DataObject = RequestBody;

	if (DataObject.IsValid())
	{
		// JSON 객체를 문자열로 인코딩하여 JSON 형식으로 출력/
		FString JsonString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
		FJsonSerializer::Serialize(DataObject.ToSharedRef(), Writer);
	}
	else
	{
		UE_LOG(SFClog, Warning, TEXT("%s : DataObject is invalid"), *this->GetName());
	}
	return DataObject;
}

// jsonObject를 map으로 변환해주는 스태틱 함수 : cpp전용
TMap<FString, FString> USFCHttpManager::ParseJsonObjToMap(const TSharedPtr<FJsonObject> OriginJsonObject)
{
	TMap<FString, FString> ParsedMap;
	for (auto& Elem : OriginJsonObject->Values)
	{
		// FJsonValue의 타입에 따라 처리
		if (Elem.Value->Type == EJson::String)
		{
			ParsedMap.Add(Elem.Key, Elem.Value->AsString());
		}
		else if (Elem.Value->Type == EJson::Number)
		{
			ParsedMap.Add(Elem.Key, FString::SanitizeFloat(Elem.Value->AsNumber()));
		}
		else if (Elem.Value->Type == EJson::Object)
		{
			// 중첩된 JSON 객체는 FString으로 변환
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParsedMap.Add(Elem.Key));
			FJsonSerializer::Serialize(Elem.Value->AsObject().ToSharedRef(), Writer);
		}
		else if (Elem.Value->Type == EJson::Array)
		{
			// 배열은 FString으로 변환
			TArray<TSharedPtr<FJsonValue>> ArrayValues = Elem.Value->AsArray();
			FString ArrayAsString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ArrayAsString);
			FJsonSerializer::Serialize(ArrayValues, Writer);
			ParsedMap.Add(Elem.Key, ArrayAsString);
		}
		else
		{
			ParsedMap.Add(Elem.Key, TEXT("")); // 기타 경우 빈 문자열
		}
	}

	return ParsedMap;
}
