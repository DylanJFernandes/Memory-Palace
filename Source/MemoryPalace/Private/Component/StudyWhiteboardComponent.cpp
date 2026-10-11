// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/StudyWhiteboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Game/StudyPadSubsystem.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr int32 WhiteboardMaterialSlotIndex{0};
}

UStudyWhiteboardComponent::UStudyWhiteboardComponent()
	: TextureParameterName{TEXT("WhiteboardTexture")}
	, bIsShowingSavedImage{false}
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudyWhiteboardComponent::BeginPlay()
{
	Super::BeginPlay();
	CreateBlankTexture();
	CreateWhiteboardMaterial();
	BindToStudyPadSubsystem();
}

void UStudyWhiteboardComponent::CreateBlankTexture()
{
	BlankTexture = UTexture2D::CreateTransient(1, 1, PF_B8G8R8A8);
	FTexture2DMipMap& BaseMip{
		BlankTexture->GetPlatformData()->Mips[0]};
	void* PixelData{BaseMip.BulkData.Lock(LOCK_READ_WRITE)};
	FMemory::Memset(PixelData, 0xFF, sizeof(FColor));
	BaseMip.BulkData.Unlock();
	BlankTexture->UpdateResource();
}

void UStudyWhiteboardComponent::CreateWhiteboardMaterial()
{
	UStaticMeshComponent* WhiteboardMesh{
		GetOwner()->FindComponentByClass<UStaticMeshComponent>()};
	if (!WhiteboardMesh)
	{
		return;
	}
	WhiteboardMaterial =
		WhiteboardMesh->CreateAndSetMaterialInstanceDynamic(
			WhiteboardMaterialSlotIndex);
}

void UStudyWhiteboardComponent::BindToStudyPadSubsystem()
{
	UStudyPadSubsystem* StudyPadSubsystem{
		GetWorld()->GetSubsystem<UStudyPadSubsystem>()};
	if (!StudyPadSubsystem)
	{
		return;
	}
	CachedStudyPadSubsystem = StudyPadSubsystem;
	StudyPadSubsystem->OnWorkImageUpdated.AddUObject(
		this, &UStudyWhiteboardComponent::HandleWorkImageUpdated);
	ShowTexture(StudyPadSubsystem->GetTextureForProblem(ProblemId));
}

void UStudyWhiteboardComponent::HandleWorkImageUpdated(
	const FString& UpdatedProblemId, UTexture2D* WorkTexture,
	float AspectRatio)
{
	if (UpdatedProblemId != ProblemId || bIsShowingSavedImage)
	{
		return;
	}
	ShowTexture(WorkTexture);
}

void UStudyWhiteboardComponent::ShowSavedImage(
	const TArray<uint8>& PngBytes)
{
	bIsShowingSavedImage = true;
	SavedImageTexture =
		FImageUtils::ImportBufferAsTexture2D(PngBytes);
	ShowTexture(SavedImageTexture);
}

void UStudyWhiteboardComponent::ShowLiveImage()
{
	bIsShowingSavedImage = false;
	SavedImageTexture = nullptr;
	if (!CachedStudyPadSubsystem.IsValid())
	{
		ShowTexture(nullptr);
		return;
	}
	ShowTexture(
		CachedStudyPadSubsystem->GetTextureForProblem(ProblemId));
}

const FString& UStudyWhiteboardComponent::GetProblemId() const
{
	return ProblemId;
}

void UStudyWhiteboardComponent::ShowTexture(UTexture2D* WorkTexture)
{
	if (!WhiteboardMaterial)
	{
		return;
	}
	UTexture2D* TextureToShow{
		WorkTexture ? WorkTexture : BlankTexture.Get()};
	WhiteboardMaterial->SetTextureParameterValue(
		TextureParameterName, TextureToShow);
}

void UStudyWhiteboardComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (CachedStudyPadSubsystem.IsValid())
	{
		CachedStudyPadSubsystem->OnWorkImageUpdated.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}
