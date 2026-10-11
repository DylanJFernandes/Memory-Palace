// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/MemoryPalaceGameMode.h"
#include "Data/StudyCurriculumDataAsset.h"
#include "Engine/World.h"
#include "Game/StudyGameState.h"
#include "Game/StudyPadSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Player/StudyPlayerState.h"

AMemoryPalaceGameMode::AMemoryPalaceGameMode()
	: ExperiencePerDifficultyPoint{50}
{
	GameStateClass = AStudyGameState::StaticClass();
	PlayerStateClass = AStudyPlayerState::StaticClass();
}

void AMemoryPalaceGameMode::BeginPlay()
{
	Super::BeginPlay();
	UnlockStartingSections();
	BindToStudyPadSubsystem();
}

void AMemoryPalaceGameMode::StartProblem(const FString& ProblemId)
{
	if (!CachedStudyPadSubsystem.IsValid())
	{
		return;
	}
	CachedStudyPadSubsystem->StartProblem(ProblemId);
	AStudyGameState* StudyGameState{GetGameState<AStudyGameState>()};
	if (!StudyGameState)
	{
		return;
	}
	StudyGameState->SetActiveProblem(ProblemId);
}

void AMemoryPalaceGameMode::UnlockStartingSections()
{
	AStudyGameState* StudyGameState{GetGameState<AStudyGameState>()};
	if (!StudyGameState || !Curriculum)
	{
		return;
	}
	for (const FStudySectionDefinition& Section :
		 Curriculum->GetSections())
	{
		if (Section.PrerequisiteSectionName.IsNone())
		{
			StudyGameState->UnlockSection(Section.SectionName);
		}
	}
}

void AMemoryPalaceGameMode::BindToStudyPadSubsystem()
{
	UStudyPadSubsystem* StudyPadSubsystem{
		GetWorld()->GetSubsystem<UStudyPadSubsystem>()};
	if (!StudyPadSubsystem)
	{
		return;
	}
	CachedStudyPadSubsystem = StudyPadSubsystem;
	StudyPadSubsystem->OnReviewFinished.AddUObject(
		this, &AMemoryPalaceGameMode::HandleReviewFinished);
	StudyPadSubsystem->StartPolling();
}

void AMemoryPalaceGameMode::HandleReviewFinished(
	const FStudyReviewResult& ReviewResult)
{
	if (!ReviewResult.bIsCorrect)
	{
		return;
	}
	AwardExperience(ReviewResult);
	UnlockEligibleSections();
}

void AMemoryPalaceGameMode::AwardExperience(
	const FStudyReviewResult& ReviewResult)
{
	AStudyPlayerState* StudyPlayerState{GetStudyPlayerState()};
	if (!StudyPlayerState)
	{
		return;
	}
	const double ScaledExperience{
		ExperiencePerDifficultyPoint * ReviewResult.DifficultyRating};
	const int64 ExperienceAmount{FMath::RoundToInt(ScaledExperience)};
	StudyPlayerState->AddExperience(
		EStudyTopicKind::Subject,
		ReviewResult.GetSubjectTopicName(), ExperienceAmount);
	StudyPlayerState->AddExperience(
		EStudyTopicKind::Section,
		ReviewResult.GetSectionTopicName(), ExperienceAmount);
}

void AMemoryPalaceGameMode::UnlockEligibleSections()
{
	AStudyGameState* StudyGameState{GetGameState<AStudyGameState>()};
	if (!StudyGameState || !Curriculum)
	{
		return;
	}
	for (const FStudySectionDefinition& Section :
		 Curriculum->GetSections())
	{
		if (IsPrerequisiteMet(Section))
		{
			StudyGameState->UnlockSection(Section.SectionName);
		}
	}
}

bool AMemoryPalaceGameMode::IsPrerequisiteMet(
	const FStudySectionDefinition& Section) const
{
	const AStudyPlayerState* StudyPlayerState{GetStudyPlayerState()};
	if (!StudyPlayerState || Section.PrerequisiteSectionName.IsNone())
	{
		return false;
	}
	const int32 PrerequisiteExperience{
		StudyPlayerState->GetExperience(
			EStudyTopicKind::Section,
			Section.PrerequisiteSectionName)};
	return PrerequisiteExperience
		>= Section.RequiredPrerequisiteExperience;
}

AStudyPlayerState* AMemoryPalaceGameMode::GetStudyPlayerState() const
{
	const APlayerController* PlayerController{
		GetWorld()->GetFirstPlayerController()};
	if (!PlayerController)
	{
		return nullptr;
	}
	return PlayerController->GetPlayerState<AStudyPlayerState>();
}

void AMemoryPalaceGameMode::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (CachedStudyPadSubsystem.IsValid())
	{
		CachedStudyPadSubsystem->OnReviewFinished.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}
