// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/StudyPadImageSubsystem.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"

namespace
{
	constexpr const TCHAR* ServerBaseUrl{
		TEXT("http://127.0.0.1:3000")};
	constexpr const TCHAR* LatestImagePath{
		TEXT("/latest.png")};
	constexpr float PollIntervalSeconds{0.5f};
	constexpr int32 HttpStatusOk{200};
}

UStudyPadImageSubsystem::UStudyPadImageSubsystem()
	: bIsRequestInFlight{false}
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
	InWorld.GetTimerManager().SetTimer(
		PollTimerHandle, this,
		&UStudyPadImageSubsystem::RequestLatestImage,
		PollIntervalSeconds, true);
}

void UStudyPadImageSubsystem::RequestLatestImage()
{
	if (bIsRequestInFlight)
	{
		return;
	}
	bIsRequestInFlight = true;
	CreateLatestImageRequest()->ProcessRequest();
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
	UStudyPadImageSubsystem::CreateLatestImageRequest()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request{
		FHttpModule::Get().CreateRequest()};
	Request->SetURL(FString{ServerBaseUrl} + LatestImagePath);
	Request->SetVerb(TEXT("GET"));
	Request->OnProcessRequestComplete().BindUObject(
		this, &UStudyPadImageSubsystem::HandleImageResponse);
	return Request;
}

void UStudyPadImageSubsystem::HandleImageResponse(
	FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	bIsRequestInFlight = false;
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

UTexture2D* UStudyPadImageSubsystem::GetLatestTexture() const
{
	return LatestTexture;
}

void UStudyPadImageSubsystem::Deinitialize()
{
	if (UWorld* World{GetWorld()})
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}
	Super::Deinitialize();
}
