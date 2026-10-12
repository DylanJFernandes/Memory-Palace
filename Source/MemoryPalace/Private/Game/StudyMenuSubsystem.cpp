// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/StudyMenuSubsystem.h"
#include "Data/StudyMenuSaveGame.h"
#include "Data/StudyServerAddress.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FString MenuSaveSlotName{TEXT("StudyMenu")};
	constexpr int32 MenuSaveUserIndex{0};
	constexpr int32 MenuHttpStatusOk{200};
	constexpr const TCHAR* ConfigPath{TEXT("/config")};
	constexpr const TCHAR* UploadUrlPath{TEXT("/lan-url.json")};
	constexpr const TCHAR* GeneratePath{TEXT("/generate")};
	constexpr const TCHAR* MaterialsStatusPath{
		TEXT("/materials/status")};

	/* Public QR code image service; it only receives the
	   upload page's local network address */
	constexpr const TCHAR* QrCodeServiceUrl{TEXT(
		"https://api.qrserver.com/v1/create-qr-code/"
		"?size=300x300&data=")};

	/* Parses a response body as a JSON object */
	TSharedPtr<FJsonObject> ParseMenuJsonResponse(
		const FHttpResponsePtr& Response)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader{
			TJsonReaderFactory<>::Create(
				Response->GetContentAsString())};
		FJsonSerializer::Deserialize(Reader, JsonObject);
		return JsonObject;
	}

	/* Returns whether a response arrived with status 200 */
	bool IsUsableMenuResponse(const FHttpResponsePtr& Response,
						  bool bConnectedSuccessfully)
	{
		return bConnectedSuccessfully && Response.IsValid()
			&& Response->GetResponseCode() == MenuHttpStatusOk;
	}
}

void UStudyMenuSubsystem::Initialize(
	FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MenuSaveGame = Cast<UStudyMenuSaveGame>(
		UGameplayStatics::LoadGameFromSlot(
			MenuSaveSlotName, MenuSaveUserIndex));
	if (!MenuSaveGame)
	{
		MenuSaveGame = Cast<UStudyMenuSaveGame>(
			UGameplayStatics::CreateSaveGameObject(
				UStudyMenuSaveGame::StaticClass()));
	}
	SendApiKeyToServer();
}

const TArray<FString>& UStudyMenuSubsystem::GetWorldNames() const
{
	return MenuSaveGame->WorldNames;
}

void UStudyMenuSubsystem::AddWorld(const FString& WorldName)
{
	MenuSaveGame->WorldNames.AddUnique(WorldName);
	SaveToDisk();
}

void UStudyMenuSubsystem::RemoveWorld(const FString& WorldName)
{
	MenuSaveGame->WorldNames.Remove(WorldName);
	SaveToDisk();
}

const FString& UStudyMenuSubsystem::GetApiKey() const
{
	return MenuSaveGame->ApiKey;
}

void UStudyMenuSubsystem::SaveApiKey(const FString& InApiKey)
{
	MenuSaveGame->ApiKey = InApiKey.TrimStartAndEnd();
	SaveToDisk();
	SendApiKeyToServer();
}

void UStudyMenuSubsystem::SaveToDisk() const
{
	UGameplayStatics::SaveGameToSlot(
		MenuSaveGame, MenuSaveSlotName, MenuSaveUserIndex);
}

void UStudyMenuSubsystem::SendApiKeyToServer() const
{
	if (MenuSaveGame->ApiKey.IsEmpty())
	{
		return;
	}
	TSharedRef<FJsonObject> ConfigObject{MakeShared<FJsonObject>()};
	ConfigObject->SetStringField(
		TEXT("apiKey"), MenuSaveGame->ApiKey);
	FString ConfigText;
	FJsonSerializer::Serialize(ConfigObject,
		TJsonWriterFactory<>::Create(&ConfigText));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateServerRequest(TEXT("POST"), ConfigPath)};
	Request->SetHeader(
		TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(ConfigText);
	Request->ProcessRequest();
}

void UStudyMenuSubsystem::RequestQrCode()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateServerRequest(TEXT("GET"), UploadUrlPath)};
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyMenuSubsystem::HandleUploadUrlResponse);
	Request->ProcessRequest();
}

void UStudyMenuSubsystem::HandleUploadUrlResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!IsUsableMenuResponse(Response, bConnectedSuccessfully))
	{
		return;
	}
	const TSharedPtr<FJsonObject> UrlObject{
		ParseMenuJsonResponse(Response)};
	if (!UrlObject.IsValid())
	{
		return;
	}
	const FString UploadUrl{
		UrlObject->GetStringField(TEXT("uploadUrl"))};
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> QrRequest{
		FHttpModule::Get().CreateRequest()};
	QrRequest->SetURL(FString{QrCodeServiceUrl}
		+ FGenericPlatformHttp::UrlEncode(UploadUrl));
	QrRequest->SetVerb(TEXT("GET"));
	QrRequest->OnProcessRequestComplete().BindUObject(
		this, &UStudyMenuSubsystem::HandleQrCodeResponse);
	QrRequest->ProcessRequest();
}

void UStudyMenuSubsystem::HandleQrCodeResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!IsUsableMenuResponse(Response, bConnectedSuccessfully))
	{
		return;
	}
	QrCodeTexture =
		FImageUtils::ImportBufferAsTexture2D(Response->GetContent());
	OnQrCodeReceived.ExecuteIfBound(QrCodeTexture);
}

void UStudyMenuSubsystem::RequestGeneration()
{
	CreateServerRequest(TEXT("POST"), GeneratePath)
		->ProcessRequest();
}

void UStudyMenuSubsystem::RequestMaterialsStatus()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateServerRequest(TEXT("GET"), MaterialsStatusPath)};
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyMenuSubsystem::HandleMaterialsStatusResponse);
	Request->ProcessRequest();
}

void UStudyMenuSubsystem::HandleMaterialsStatusResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!IsUsableMenuResponse(Response, bConnectedSuccessfully))
	{
		OnMaterialsStatusReceived.ExecuteIfBound(0.0f,
			TEXT("offline"), TEXT("Can't reach the study server."));
		return;
	}
	const TSharedPtr<FJsonObject> StatusObject{
		ParseMenuJsonResponse(Response)};
	if (!StatusObject.IsValid())
	{
		return;
	}
	OnMaterialsStatusReceived.ExecuteIfBound(
		static_cast<float>(
			StatusObject->GetNumberField(TEXT("progress"))),
		StatusObject->GetStringField(TEXT("status")),
		StatusObject->GetStringField(TEXT("message")));
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
	UStudyMenuSubsystem::CreateServerRequest(
		const TCHAR* Verb, const FString& Path)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		FHttpModule::Get().CreateRequest()};
	Request->SetURL(FString{StudyServerBaseUrl} + Path);
	Request->SetVerb(Verb);
	return Request;
}
