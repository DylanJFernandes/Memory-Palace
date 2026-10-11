// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/MemoryPalaceGameMode.h"
#include "Engine/World.h"
#include "Game/StudyGameState.h"
#include "Game/StudyPadSubsystem.h"
#include "Game/StudyProgressSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Player/StudyPlayerState.h"

AMemoryPalaceGameMode::AMemoryPalaceGameMode()
	: ExperiencePerDifficultyPoint{50}
	, MinimumRepeatFraction{0.1f}
	, RepeatRecoverySeconds{3600.0f}
	, UnlockExperienceThreshold{10}
{
	GameStateClass = AStudyGameState::StaticClass();
	PlayerStateClass = AStudyPlayerState::StaticClass();
}

void AMemoryPalaceGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	AStudyPlayerState* StudyPlayerState{
		NewPlayer->GetPlayerState<AStudyPlayerState>()};
	UStudyProgressSubsystem* ProgressSubsystem{
		GetWorld()->GetSubsystem<UStudyProgressSubsystem>()};
	if (!StudyPlayerState || !ProgressSubsystem)
	{
		return;
	}
	ProgressSubsystem->RestoreExperience(*StudyPlayerState);
	UnlockEligibleSections();
}

void AMemoryPalaceGameMode::BeginPlay()
{
	Super::BeginPlay();
	CachedProgressSubsystem =
		GetWorld()->GetSubsystem<UStudyProgressSubsystem>();
	BindToStudyPadSubsystem();
}

void AMemoryPalaceGameMode::StartProblem(const FString& ProblemId)
{
	if (!CachedStudyPadSubsystem.IsValid())
	{
		return;
	}
	RewardedPartKeysInAttempt.Empty();
	SaveBoardsToDisk();
	CachedStudyPadSubsystem->StartProblem(ProblemId);
	AStudyGameState* StudyGameState{GetGameState<AStudyGameState>()};
	if (!StudyGameState)
	{
		return;
	}
	StudyGameState->SetActiveProblem(ProblemId);
}

void AMemoryPalaceGameMode::RequestHint()
{
	if (!CachedStudyPadSubsystem.IsValid())
	{
		return;
	}
	CachedStudyPadSubsystem->RequestHint();
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
	StudyPadSubsystem->OnCurriculumLoaded.AddUObject(
		this, &AMemoryPalaceGameMode::HandleCurriculumLoaded);
	StudyPadSubsystem->StartPolling();
	StudyPadSubsystem->RequestCurriculum();
}

void AMemoryPalaceGameMode::HandleCurriculumLoaded(
	const FStudyCurriculum& LoadedCurriculum)
{
	Curriculum = LoadedCurriculum;
	UnlockEligibleSections();
}

void AMemoryPalaceGameMode::HandleReviewFinished(
	const FStudyReviewResult& ReviewResult)
{
	const int32 ExperienceGained{AwardExperience(ReviewResult)};
	UnlockEligibleSections();
	SaveProgress(ReviewResult, ExperienceGained);
}

void AMemoryPalaceGameMode::SaveBoardsToDisk() const
{
	if (!CachedProgressSubsystem.IsValid())
	{
		return;
	}
	CachedProgressSubsystem->SaveToDisk();
}

void AMemoryPalaceGameMode::SaveProgress(
	const FStudyReviewResult& ReviewResult, int32 ExperienceGained)
{
	const AStudyPlayerState* StudyPlayerState{GetStudyPlayerState()};
	if (!CachedProgressSubsystem.IsValid() || !StudyPlayerState)
	{
		return;
	}
	CachedProgressSubsystem->RecordSubmission(
		ReviewResult, ExperienceGained);
	CachedProgressSubsystem->StoreExperience(*StudyPlayerState);
	CachedProgressSubsystem->SaveToDisk();
}

int32 AMemoryPalaceGameMode::AwardExperience(
	const FStudyReviewResult& ReviewResult)
{
	AStudyPlayerState* StudyPlayerState{GetStudyPlayerState()};
	if (!StudyPlayerState || ReviewResult.Parts.IsEmpty())
	{
		return 0;
	}
	const double EarnedExperience{
		ClaimCorrectPartsExperience(ReviewResult, *StudyPlayerState)};
	const int64 ExperienceAmount{FMath::RoundToInt(EarnedExperience)};
	if (ExperienceAmount <= 0)
	{
		return 0;
	}
	StudyPlayerState->AddExperience(
		EStudyTopicKind::Subject,
		ReviewResult.GetSubjectTopicName(), ExperienceAmount);
	StudyPlayerState->AddExperience(
		EStudyTopicKind::Section,
		ReviewResult.GetSectionTopicName(), ExperienceAmount);
	return static_cast<int32>(ExperienceAmount);
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
		if (RewardedPartKeysInAttempt.Contains(PartKey))
		{
			continue;
		}
		RewardedPartKeysInAttempt.Add(PartKey);
		const double Freshness{
			ComputeFreshness(StudyPlayerState, PartKey)};
		EarnedExperience += PartExperience * Freshness;
		StudyPlayerState.RecordPartAward(
			PartKey, GetStudyNow());
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
		(GetStudyNow() - LastAwardTime.GetValue())
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
	if (!StudyGameState)
	{
		return;
	}
	for (int32 SectionIndex{0};
		 SectionIndex < Curriculum.Sections.Num(); ++SectionIndex)
	{
		if (IsPreviousSectionMastered(SectionIndex))
		{
			StudyGameState->UnlockSection(FName{
				Curriculum.Sections[SectionIndex].SectionName});
		}
	}
}

bool AMemoryPalaceGameMode::IsPreviousSectionMastered(
	int32 SectionIndex) const
{
	if (SectionIndex == 0)
	{
		return true;
	}
	const AStudyPlayerState* StudyPlayerState{GetStudyPlayerState()};
	if (!StudyPlayerState)
	{
		return false;
	}
	const FName PreviousSectionName{
		Curriculum.Sections[SectionIndex - 1].SectionName};
	return StudyPlayerState->GetExperience(
			   EStudyTopicKind::Section, PreviousSectionName)
		>= UnlockExperienceThreshold;
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

FDateTime AMemoryPalaceGameMode::GetStudyNow() const
{
	const AStudyGameState* StudyGameState{
		GetGameState<AStudyGameState>()};
	if (!StudyGameState)
	{
		return FDateTime::UtcNow();
	}
	return StudyGameState->GetStudyNow();
}
