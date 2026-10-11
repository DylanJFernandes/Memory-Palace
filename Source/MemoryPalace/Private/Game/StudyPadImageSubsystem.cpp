// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/StudyPadImageSubsystem.h"
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

UStudyPadImageSubsystem::UStudyPadImageSubsystem()
	: bIsImageRequestInFlight{false}
	, bIsResultRequestInFlight{false}
	, CurrentReviewStatus{EStudyReviewStatus::Idle}
{
}

bool UStudyPadImageSubsystem::ShouldCreateSubsystem(
	UObject* Outer) const
{
	const UWorld* World{Cast<UWorld>(Outer)};
	if (!World || !World->IsGameWorld())
	{
		return false;
	}
	return Super::ShouldCreateSubsystem(Outer);
}

void UStudyPadImageSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FTimerManager& TimerManager{InWorld.GetTimerManager()};
	TimerManager.SetTimer(
		ImagePollTimerHandle, this,
		&UStudyPadImageSubsystem::RequestLatestImage,
		ImagePollIntervalSeconds, true);
	TimerManager.SetTimer(
		ResultPollTimerHandle, this,
		&UStudyPadImageSubsystem::RequestLatestResult,
		ResultPollIntervalSeconds, true);
}

void UStudyPadImageSubsystem::RequestLatestImage()
{
	if (bIsImageRequestInFlight)
	{
		return;
	}
	bIsImageRequestInFlight = true;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateGetRequest(LatestImagePath)};
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyPadImageSubsystem::HandleImageResponse);
	Request->ProcessRequest();
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
	UStudyPadImageSubsystem::CreateGetRequest(const TCHAR* Path)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		FHttpModule::Get().CreateRequest()};
	Request->SetURL(FString{ServerBaseUrl} + Path);
	Request->SetVerb(TEXT("GET"));
	return Request;
}

void UStudyPadImageSubsystem::HandleImageResponse(
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

void UStudyPadImageSubsystem::PublishImage(
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

void UStudyPadImageSubsystem::RequestLatestResult()
{
	if (bIsResultRequestInFlight)
	{
		return;
	}
	bIsResultRequestInFlight = true;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateGetRequest(LatestResultPath)};
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyPadImageSubsystem::HandleResultResponse);
	Request->ProcessRequest();
}

void UStudyPadImageSubsystem::HandleResultResponse(
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

void UStudyPadImageSubsystem::HandleResultUnavailable()
{
	if (CurrentReviewStatus != EStudyReviewStatus::Reviewing)
	{
		return;
	}
	CurrentReviewStatus = EStudyReviewStatus::Failed;
	OnReviewFailed.Broadcast(
		TEXT("Lost contact with the study pad server."));
}

void UStudyPadImageSubsystem::ApplyServerStatus(
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

EStudyReviewStatus UStudyPadImageSubsystem::ParseStatus(
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

void UStudyPadImageSubsystem::BroadcastStatusChange(
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

UTexture2D* UStudyPadImageSubsystem::GetLatestTexture() const
{
	return LatestTexture;
}

void UStudyPadImageSubsystem::Deinitialize()
{
	if (UWorld* World{GetWorld()})
	{
		FTimerManager& TimerManager{World->GetTimerManager()};
		TimerManager.ClearTimer(ImagePollTimerHandle);
		TimerManager.ClearTimer(ResultPollTimerHandle);
	}
	Super::Deinitialize();
}
