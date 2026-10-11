// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/StudyPadSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Crc.h"
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
		TEXT("/result.json?source=game")};
	constexpr const TCHAR* StartProblemPath{
		TEXT("/start?problem=")};
	constexpr const TCHAR* EndProblemPath{
		TEXT("/end")};
	constexpr const TCHAR* HintPath{
		TEXT("/hint")};
	constexpr float HintTimeoutSeconds{60.0f};
	constexpr int32 HttpStatusConflict{409};
	constexpr float ImagePollIntervalSeconds{1.0f};
	constexpr float ResultPollIntervalSeconds{0.7f};
	constexpr int32 HttpStatusOk{200};
	const FString ProblemIdHeaderName{TEXT("X-Problem-Id")};
}

UStudyPadSubsystem::UStudyPadSubsystem()
	: bIsPolling{false}
	, bIsImageRequestInFlight{false}
	, bIsResultRequestInFlight{false}
	, bHasPolledResult{false}
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

void UStudyPadSubsystem::StartPolling()
{
	bIsPolling = true;
	FTimerManager& TimerManager{GetWorld()->GetTimerManager()};
	TimerManager.SetTimer(
		ImagePollTimerHandle, this,
		&UStudyPadSubsystem::RequestLatestImage,
		ImagePollIntervalSeconds, true);
	TimerManager.SetTimer(
		ResultPollTimerHandle, this,
		&UStudyPadSubsystem::RequestLatestResult,
		ResultPollIntervalSeconds, true);
}

void UStudyPadSubsystem::StartProblem(const FString& ProblemId)
{
	const FString EncodedProblemId{
		FGenericPlatformHttp::UrlEncode(ProblemId)};
	const FString Path{StartProblemPath + EncodedProblemId};
	CreateServerRequest(TEXT("POST"), Path)->ProcessRequest();
}

void UStudyPadSubsystem::RequestLatestImage()
{
	if (bIsImageRequestInFlight)
	{
		return;
	}
	bIsImageRequestInFlight = true;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateServerRequest(TEXT("GET"), LatestImagePath)};
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyPadSubsystem::HandleImageResponse);
	Request->ProcessRequest();
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
	UStudyPadSubsystem::CreateServerRequest(
		const TCHAR* Verb, const FString& Path)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		FHttpModule::Get().CreateRequest()};
	Request->SetURL(FString{ServerBaseUrl} + Path);
	Request->SetVerb(Verb);
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
	const FString ProblemId{
		Response->GetHeader(ProblemIdHeaderName)};
	if (ProblemId.IsEmpty())
	{
		return;
	}
	PublishImage(ProblemId, Response->GetContent());
}

void UStudyPadSubsystem::PublishImage(
	const FString& ProblemId, const TArray<uint8>& ImageBytes)
{
	const uint32 Checksum{
		FCrc::MemCrc32(ImageBytes.GetData(), ImageBytes.Num())};
	const uint32* LastChecksum{LastImageChecksums.Find(ProblemId)};
	if (LastChecksum && *LastChecksum == Checksum)
	{
		return;
	}
	UTexture2D* ImportedTexture{
		FImageUtils::ImportBufferAsTexture2D(ImageBytes)};
	if (!ImportedTexture)
	{
		return;
	}
	LastImageChecksums.Add(ProblemId, Checksum);
	ProblemTextures.Add(ProblemId, ImportedTexture);
	const float AspectRatio{static_cast<float>(
		ImportedTexture->GetSizeX()) / ImportedTexture->GetSizeY()};
	OnWorkImageUpdated.Broadcast(
		ProblemId, ImportedTexture, AspectRatio);
}

void UStudyPadSubsystem::RequestLatestResult()
{
	if (bIsResultRequestInFlight)
	{
		return;
	}
	bIsResultRequestInFlight = true;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateServerRequest(TEXT("GET"), LatestResultPath)};
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
	const TSharedPtr<FJsonObject> ResultObject{
		ParseJsonObject(Response->GetContentAsString())};
	if (!ResultObject.IsValid())
	{
		HandleResultUnavailable();
		return;
	}
	ApplyServerStatus(*ResultObject);
}

TSharedPtr<FJsonObject> UStudyPadSubsystem::ParseJsonObject(
	const FString& JsonText)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader{
		TJsonReaderFactory<>::Create(JsonText)};
	FJsonSerializer::Deserialize(Reader, JsonObject);
	return JsonObject;
}

void UStudyPadSubsystem::RequestHint()
{
	OnHintUpdated.ExecuteIfBound(TEXT("Thinking of a hint..."));
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		CreateServerRequest(TEXT("POST"), HintPath)};
	Request->SetTimeout(HintTimeoutSeconds);
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyPadSubsystem::HandleHintResponse);
	Request->ProcessRequest();
}

void UStudyPadSubsystem::HandleHintResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	const bool bHasHint{
		bConnectedSuccessfully && Response.IsValid()
		&& Response->GetResponseCode() == HttpStatusOk};
	if (!bHasHint)
	{
		const FString FailureMessage{BuildHintFailureMessage(
			Response, bConnectedSuccessfully)};
		OnHintUpdated.ExecuteIfBound(FailureMessage);
		return;
	}
	FString HintText;
	const TSharedPtr<FJsonObject> HintObject{
		ParseJsonObject(Response->GetContentAsString())};
	if (HintObject.IsValid())
	{
		HintObject->TryGetStringField(TEXT("hint"), HintText);
	}
	OnHintUpdated.ExecuteIfBound(HintText);
}

FString UStudyPadSubsystem::BuildHintFailureMessage(
	FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		return TEXT("Couldn't reach the study pad server.");
	}
	if (Response->GetResponseCode() == HttpStatusConflict)
	{
		return TEXT("Start a problem at a station first.");
	}
	return TEXT("Couldn't get a hint right now.");
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
	if (NewStatus == EStudyReviewStatus::Reviewing)
	{
		HandleReviewing();
	}
	else
	{
		CurrentReviewStatus = NewStatus;
		HandleOutcome(ResultObject);
	}
	bHasPolledResult = true;
}

void UStudyPadSubsystem::HandleReviewing()
{
	if (CurrentReviewStatus == EStudyReviewStatus::Reviewing)
	{
		return;
	}
	CurrentReviewStatus = EStudyReviewStatus::Reviewing;
	OnReviewStarted.Broadcast();
}

void UStudyPadSubsystem::HandleOutcome(
	const FJsonObject& ResultObject)
{
	if (CurrentReviewStatus == EStudyReviewStatus::Idle)
	{
		return;
	}
	const FString ResultKey{BuildResultKey(ResultObject)};
	if (ResultKey == LastHandledResultKey)
	{
		return;
	}
	LastHandledResultKey = ResultKey;
	if (bHasPolledResult)
	{
		BroadcastOutcome(ResultObject);
	}
}

FString UStudyPadSubsystem::BuildResultKey(
	const FJsonObject& ResultObject)
{
	FString ProblemId;
	ResultObject.TryGetStringField(TEXT("problemId"), ProblemId);
	double ElapsedSeconds{0.0};
	ResultObject.TryGetNumberField(
		TEXT("elapsedSeconds"), ElapsedSeconds);
	const FString ElapsedText{FString::SanitizeFloat(ElapsedSeconds)};
	return ProblemId + TEXT(":") + ElapsedText;
}

void UStudyPadSubsystem::BroadcastOutcome(
	const FJsonObject& ResultObject)
{
	if (CurrentReviewStatus == EStudyReviewStatus::Finished)
	{
		OnReviewFinished.Broadcast(
			FStudyReviewResult::FromJson(ResultObject));
		return;
	}
	FString ErrorMessage;
	ResultObject.TryGetStringField(TEXT("message"), ErrorMessage);
	OnReviewFailed.Broadcast(ErrorMessage);
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

UTexture2D* UStudyPadSubsystem::GetTextureForProblem(
	const FString& ProblemId) const
{
	const TObjectPtr<UTexture2D>* FoundTexture{
		ProblemTextures.Find(ProblemId)};
	return FoundTexture ? FoundTexture->Get() : nullptr;
}

void UStudyPadSubsystem::EndProblem()
{
	CreateServerRequest(TEXT("POST"), EndProblemPath)
		->ProcessRequest();
}

void UStudyPadSubsystem::Deinitialize()
{
	if (UWorld* World{GetWorld()})
	{
		FTimerManager& TimerManager{World->GetTimerManager()};
		TimerManager.ClearTimer(ImagePollTimerHandle);
		TimerManager.ClearTimer(ResultPollTimerHandle);
	}
	if (bIsPolling)
	{
		EndProblem();
	}
	Super::Deinitialize();
}
