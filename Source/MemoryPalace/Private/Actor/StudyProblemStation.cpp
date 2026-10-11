// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actor/StudyProblemStation.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Game/MemoryPalaceGameMode.h"

namespace
{
	constexpr double TriggerHalfWidth{150.0};
	constexpr double TriggerHalfHeight{100.0};
}

AStudyProblemStation::AStudyProblemStation()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(
		TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector{
		TriggerHalfWidth, TriggerHalfWidth, TriggerHalfHeight});
	TriggerBox->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	TriggerBox->SetGenerateOverlapEvents(true);

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(TriggerBox);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AStudyProblemStation::Interact()
{
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
}
