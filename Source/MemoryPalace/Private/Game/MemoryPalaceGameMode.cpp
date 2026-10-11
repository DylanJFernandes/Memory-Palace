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
	, MinimumRepeatFraction{0.1f}
	, RepeatRecoverySeconds{3600.0f}
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
	AwardExperience(ReviewResult);
	UnlockEligibleSections();
}

void AMemoryPalaceGameMode::AwardExperience(
	const FStudyReviewResult& ReviewResult)
{
	AStudyPlayerState* StudyPlayerState{GetStudyPlayerState()};
	if (!StudyPlayerState || ReviewResult.Parts.IsEmpty())
	{
		return;
	}
	const double EarnedExperience{
		ClaimCorrectPartsExperience(ReviewResult, *StudyPlayerState)};
	const int32 ExperienceAmount{FMath::RoundToInt(EarnedExperience)};
	if (ExperienceAmount <= 0)
	{
		return;
	}
	StudyPlayerState->AddExperience(
		EStudyTopicKind::Subject,
		ReviewResult.GetSubjectTopicName(), ExperienceAmount);
	StudyPlayerState->AddExperience(
		EStudyTopicKind::Section,
		ReviewResult.GetSectionTopicName(), ExperienceAmount);
}

double AMemoryPalaceGameMode::ClaimCorrectPartsExperience(
	const FStudyReviewResult& ReviewResult,
	AStudyPlayerState& StudyPlayerState)
{
	const double ProblemExperience{
		ExperiencePerDifficultyPoint * ReviewResult.DifficultyRating};
	const double PartExperience{
		ProblemExperience / ReviewResult.Parts.Num()};
	double EarnedExperience{0.0};
	for (const FStudyPartResult& Part : ReviewResult.Parts)
	{
		if (!Part.bIsCorrect)
		{
			continue;
		}
		const FString PartKey{
			ReviewResult.ProblemId + TEXT(":") + Part.Id};
		const double Freshness{
			ComputeFreshness(StudyPlayerState, PartKey)};
		EarnedExperience += PartExperience * Freshness;
		StudyPlayerState.RecordPartAward(
			PartKey, FDateTime::UtcNow());
	}
	return EarnedExperience;
}

double AMemoryPalaceGameMode::ComputeFreshness(
	const AStudyPlayerState& StudyPlayerState,
	const FString& PartKey) const
{
	const TOptional<FDateTime> LastAwardTime{
		StudyPlayerState.GetLastPartAwardTime(PartKey)};
	if (!LastAwardTime.IsSet() || RepeatRecoverySeconds <= 0.0f)
	{
		return 1.0;
	}
	const double SecondsSinceAward{
		(FDateTime::UtcNow() - LastAwardTime.GetValue())
			.GetTotalSeconds()};
	const double RecoveredFraction{FMath::Clamp(
		SecondsSinceAward / RepeatRecoverySeconds, 0.0, 1.0)};
	return FMath::Lerp(
		static_cast<double>(MinimumRepeatFraction), 1.0,
		RecoveredFraction);
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
