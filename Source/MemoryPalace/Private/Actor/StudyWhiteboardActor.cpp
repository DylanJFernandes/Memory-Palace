// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actor/StudyWhiteboardActor.h"
#include "Component/StudyHistoryComponent.h"
#include "Component/StudyWhiteboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Game/MemoryPalaceGameMode.h"
#include "Player/MemoryPalacePlayerController.h"
#include "TimerManager.h"

namespace
{
	constexpr float RedrawIntervalSeconds{0.05f};
	constexpr double TriggerHalfWidth{300.0};
	constexpr double TriggerHalfHeight{150.0};
}

AStudyWhiteboardActor::AStudyWhiteboardActor()
	: bIsCursorHeld{false}
	, bIsPlayerInside{false}
{
	PrimaryActorTick.bCanEverTick = false;

	WhiteboardMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("WhiteboardMesh"));
	SetRootComponent(WhiteboardMesh);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(
		TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(WhiteboardMesh);
	TriggerBox->SetBoxExtent(FVector{
		TriggerHalfWidth, TriggerHalfWidth, TriggerHalfHeight});
	TriggerBox->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	TriggerBox->SetGenerateOverlapEvents(true);

	WhiteboardComponent =
		CreateDefaultSubobject<UStudyWhiteboardComponent>(
			TEXT("WhiteboardComponent"));

	StudyPageWidgetComponent =
		CreateDefaultSubobject<UWidgetComponent>(
			TEXT("StudyPageWidgetComponent"));
	StudyPageWidgetComponent->SetupAttachment(WhiteboardMesh);
	StudyPageWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	StudyPageWidgetComponent->SetVisibility(false);
	StudyPageWidgetComponent->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);

	HistoryWidgetComponent =
		CreateDefaultSubobject<UWidgetComponent>(
			TEXT("HistoryWidgetComponent"));
	HistoryWidgetComponent->SetupAttachment(WhiteboardMesh);
	HistoryWidgetComponent->SetWidgetSpace(EWidgetSpace::World);

	HistoryComponent = CreateDefaultSubobject<UStudyHistoryComponent>(
		TEXT("HistoryComponent"));
}

void AStudyWhiteboardActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureCollision();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(
		this, &AStudyWhiteboardActor::HandleTriggerBeginOverlap);
	TriggerBox->OnComponentEndOverlap.AddDynamic(
		this, &AStudyWhiteboardActor::HandleTriggerEndOverlap);
	BindToCursorEvents();
	CheckPlayerAlreadyInside();
	HistoryComponent->OnStartNewRequested.BindUObject(
		this, &AStudyWhiteboardActor::Interact);
	HistoryComponent->Initialize(
		HistoryWidgetComponent, WhiteboardComponent);
}

void AStudyWhiteboardActor::ConfigureCollision()
{
	TriggerBox->SetCollisionResponseToChannel(
		ECC_Visibility, ECR_Ignore);
	HistoryWidgetComponent->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);
	HistoryWidgetComponent->SetCollisionResponseToChannel(
		ECC_Visibility, ECR_Block);
}

void AStudyWhiteboardActor::BindToCursorEvents()
{
	CachedPlayerController = Cast<AMemoryPalacePlayerController>(
		GetWorld()->GetFirstPlayerController());
	if (!CachedPlayerController.IsValid())
	{
		return;
	}
	CachedPlayerController->OnCursorShown.AddUObject(
		this, &AStudyWhiteboardActor::HandleCursorShown);
	CachedPlayerController->OnCursorHidden.AddUObject(
		this, &AStudyWhiteboardActor::HandleCursorHidden);
}

void AStudyWhiteboardActor::CheckPlayerAlreadyInside()
{
	const APawn* PlayerPawn{
		CachedPlayerController.IsValid()
			? CachedPlayerController->GetPawn() : nullptr};
	bIsPlayerInside =
		PlayerPawn && TriggerBox->IsOverlappingActor(PlayerPawn);
}

void AStudyWhiteboardActor::Interact()
{
	const FString& ProblemId{WhiteboardComponent->GetProblemId()};
	if (ProblemId.IsEmpty())
	{
		return;
	}
	AMemoryPalaceGameMode* GameMode{
		GetWorld()->GetAuthGameMode<AMemoryPalaceGameMode>()};
	if (!GameMode)
	{
		return;
	}
	GameMode->StartProblem(ProblemId);
	HistoryComponent->ShowNewestPage();
}

void AStudyWhiteboardActor::ShowPreviousPage()
{
	HistoryComponent->ShowPreviousPage();
}

void AStudyWhiteboardActor::ShowNextPage()
{
	HistoryComponent->ShowNextPage();
}

void AStudyWhiteboardActor::HandleCursorShown()
{
	bIsCursorHeld = true;
	UpdateStudyPageVisibility();
}

void AStudyWhiteboardActor::HandleCursorHidden()
{
	bIsCursorHeld = false;
	UpdateStudyPageVisibility();
}

void AStudyWhiteboardActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsLocalPlayerPawn(OtherActor))
	{
		return;
	}
	bIsPlayerInside = true;
	UpdateStudyPageVisibility();
}

void AStudyWhiteboardActor::HandleTriggerEndOverlap(
	UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (!IsLocalPlayerPawn(OtherActor))
	{
		return;
	}
	bIsPlayerInside = false;
	UpdateStudyPageVisibility();
	HistoryComponent->ShowNewestPage();
}

bool AStudyWhiteboardActor::IsLocalPlayerPawn(
	const AActor* Actor) const
{
	return CachedPlayerController.IsValid()
		&& Actor == CachedPlayerController->GetPawn();
}

void AStudyWhiteboardActor::UpdateStudyPageVisibility()
{
	if (bIsCursorHeld && bIsPlayerInside)
	{
		ShowStudyPage();
		return;
	}
	HideStudyPage();
}

void AStudyWhiteboardActor::ShowStudyPage()
{
	StudyPageWidgetComponent->SetVisibility(true);
	StudyPageWidgetComponent->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);
	StudyPageWidgetComponent->SetCollisionResponseToChannel(
		ECC_Visibility, ECR_Block);
	RequestStudyPageRedraw();
	GetWorldTimerManager().SetTimer(
		RedrawTimerHandle, this,
		&AStudyWhiteboardActor::RequestStudyPageRedraw,
		RedrawIntervalSeconds, true);
}

void AStudyWhiteboardActor::HideStudyPage()
{
	GetWorldTimerManager().ClearTimer(RedrawTimerHandle);
	StudyPageWidgetComponent->SetVisibility(false);
	StudyPageWidgetComponent->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
}

void AStudyWhiteboardActor::RequestStudyPageRedraw()
{
	StudyPageWidgetComponent->RequestRedraw();
}

void AStudyWhiteboardActor::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (CachedPlayerController.IsValid())
	{
		CachedPlayerController->OnCursorShown.RemoveAll(this);
		CachedPlayerController->OnCursorHidden.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}
