// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/StudyPadSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

namespace
{
	constexpr const TCHAR* ServerBaseUrl{
		TEXT("http://127.0.0.1:3000")};
	constexpr const TCHAR* LatestImagePath{
		TEXT("/latest.png")};
	constexpr const TCHAR* LatestResultPath{
		TEXT("/result.json")};
	constexpr float ImagePollIntervalSeconds{0.5f};
	constexpr float ResultPollIntervalSeconds{0.7f};
	constexpr int32 HttpStatusOk{200};
}

UStudyPadSubsystem::UStudyPadSubsystem()
	: bIsImageRequestInFlight{false}
	, bIsResultRequestInFlight{false}
	, CurrentReviewStatus{EStudyReviewStatus::Idle}
{
}

bool UStudyPadSubsystem::ShouldCreateSubsystem(
	UObject* Outer) const
{
	const UWorld* World{Cast<UWorld>(Outer)};
	if (!World || !World->IsGameWorld())
	{
		return false;
	}
	return Super::ShouldCreateSubsystem(Outer);
}

void UStudyPadSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FTimerManager& TimerManager{InWorld.GetTimerManager()};
	TimerManager.SetTimer(
		ImagePollTimerHandle, this,
		&UStudyPadSubsystem::RequestLatestImage,
		ImagePollIntervalSeconds, true);
	TimerManager.SetTimer(
		ResultPollTimerHandle, this,
		&UStudyPadSubsystem::RequestLatestResult,
		ResultPollIntervalSeconds, true);
}

void UStudyPadSubsystem::RequestLatestImage()
{
	if (bIsImageRequestInFlight)
	{
		return;
	}
	bIsImageRequestInFlight = true;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateGetRequest(LatestImagePath)};
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyPadSubsystem::HandleImageResponse);
	Request->ProcessRequest();
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
	UStudyPadSubsystem::CreateGetRequest(const TCHAR* Path)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		FHttpModule::Get().CreateRequest()};
	Request->SetURL(FString{ServerBaseUrl} + Path);
	Request->SetVerb(TEXT("GET"));
	return Request;
}

void UStudyPadSubsystem::HandleImageResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	bIsImageRequestInFlight = false;
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		return;
	}
	if (Response->GetResponseCode() != HttpStatusOk)
	{
		return;
	}
	PublishImage(Response->GetContent());
}

void UStudyPadSubsystem::PublishImage(
	const TArray<uint8>& ImageBytes)
{
	UTexture2D* ImportedTexture{
		FImageUtils::ImportBufferAsTexture2D(ImageBytes)};
	if (!ImportedTexture)
	{
		return;
	}
	LatestTexture = ImportedTexture;
	OnImageReceived.ExecuteIfBound(LatestTexture);
}

void UStudyPadSubsystem::RequestLatestResult()
{
	if (bIsResultRequestInFlight)
	{
		return;
	}
	bIsResultRequestInFlight = true;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateGetRequest(LatestResultPath)};
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyPadSubsystem::HandleResultResponse);
	Request->ProcessRequest();
}

void UStudyPadSubsystem::HandleResultResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	bIsResultRequestInFlight = false;
	bool bHasUsableResponse{
		bConnectedSuccessfully && Response.IsValid()
		&& Response->GetResponseCode() == HttpStatusOk};
	if (!bHasUsableResponse)
	{
		HandleResultUnavailable();
		return;
	}
	TSharedPtr<FJsonObject> ResultObject;
	TSharedRef<TJsonReader<>> Reader{
		TJsonReaderFactory<>::Create(Response->GetContentAsString())};
	if (!FJsonSerializer::Deserialize(Reader, ResultObject))
	{
		HandleResultUnavailable();
		return;
	}
	ApplyServerStatus(*ResultObject);
}

void UStudyPadSubsystem::HandleResultUnavailable()
{
	if (CurrentReviewStatus != EStudyReviewStatus::Reviewing)
	{
		return;
	}
	CurrentReviewStatus = EStudyReviewStatus::Failed;
	OnReviewFailed.Broadcast(
		TEXT("Lost contact with the study pad server."));
}

void UStudyPadSubsystem::ApplyServerStatus(
	const FJsonObject& ResultObject)
{
	FString StatusText;
	ResultObject.TryGetStringField(TEXT("status"), StatusText);
	const EStudyReviewStatus NewStatus{ParseStatus(StatusText)};
	if (NewStatus == CurrentReviewStatus)
	{
		return;
	}
	CurrentReviewStatus = NewStatus;
	BroadcastStatusChange(ResultObject);
}

EStudyReviewStatus UStudyPadSubsystem::ParseStatus(
	const FString& StatusText)
{
	if (StatusText == TEXT("reviewing"))
	{
		return EStudyReviewStatus::Reviewing;
	}
	if (StatusText == TEXT("done") || StatusText == TEXT("stub"))
	{
		return EStudyReviewStatus::Finished;
	}
	if (StatusText == TEXT("error"))
	{
		return EStudyReviewStatus::Failed;
	}
	return EStudyReviewStatus::Idle;
}

void UStudyPadSubsystem::BroadcastStatusChange(
	const FJsonObject& ResultObject)
{
	switch (CurrentReviewStatus)
	{
	case EStudyReviewStatus::Reviewing:
		OnReviewStarted.Broadcast();
		break;
	case EStudyReviewStatus::Finished:
		OnReviewFinished.Broadcast(
			FStudyReviewResult::FromJson(ResultObject));
		break;
	case EStudyReviewStatus::Failed:
		OnReviewFailed.Broadcast(
			ResultObject.GetStringField(TEXT("message")));
		break;
	default:
		break;
	}
}

UTexture2D* UStudyPadSubsystem::GetLatestTexture() const
{
	return LatestTexture;
}

void UStudyPadSubsystem::Deinitialize()
{
	if (UWorld* World{GetWorld()})
	{
		FTimerManager& TimerManager{World->GetTimerManager()};
		TimerManager.ClearTimer(ImagePollTimerHandle);
		TimerManager.ClearTimer(ResultPollTimerHandle);
	}
	Super::Deinitialize();
}
