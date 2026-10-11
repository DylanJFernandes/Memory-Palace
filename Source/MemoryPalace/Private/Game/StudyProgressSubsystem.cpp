// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/StudyProgressSubsystem.h"
#include "Data/StudyProgressSaveGame.h"
#include "Data/StudyReviewResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Player/StudyPlayerState.h"

namespace
{
	constexpr int32 SaveUserIndex{0};
	const FString SlotNamePrefix{TEXT("StudyProgress_")};
}

bool UStudyProgressSubsystem::ShouldCreateSubsystem(
	UObject* Outer) const
{
	const UWorld* World{Cast<UWorld>(Outer)};
	if (!World || !World->IsGameWorld())
	{
		return false;
	}
	return Super::ShouldCreateSubsystem(Outer);
}

void UStudyProgressSubsystem::Initialize(
	FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const FString SlotName{BuildSlotName()};
	ProgressSave = Cast<UStudyProgressSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
	if (!ProgressSave)
	{
		ProgressSave = Cast<UStudyProgressSaveGame>(
			UGameplayStatics::CreateSaveGameObject(
				UStudyProgressSaveGame::StaticClass()));
	}
}

FString UStudyProgressSubsystem::BuildSlotName() const
{
	const FString MapName{
		UWorld::RemovePIEPrefix(GetWorld()->GetMapName())};
	return SlotNamePrefix + MapName;
}

void UStudyProgressSubsystem::RecordSubmission(
	const FStudyReviewResult& ReviewResult, int32 ExperienceGained)
{
	FStudySavedProblem& Problem{
		ProgressSave->Problems.FindOrAdd(ReviewResult.ProblemId)};
	FStudySavedSubmission Submission{
		BuildSubmission(ReviewResult, ExperienceGained)};
	Submission.BoardImagePngBytes = Problem.BoardImagePngBytes;
	Problem.Submissions.Add(MoveTemp(Submission));
	Problem.SubjectName = ReviewResult.SubjectName;
	Problem.SectionName = ReviewResult.SectionName;
	Problem.bIsSolved = Problem.bIsSolved || ReviewResult.bIsCorrect;
	OnSubmissionRecorded.Broadcast(ReviewResult.ProblemId);
}

FStudySavedSubmission UStudyProgressSubsystem::BuildSubmission(
	const FStudyReviewResult& ReviewResult, int32 ExperienceGained)
{
	FStudySavedSubmission Submission;
	Submission.PartCount = ReviewResult.Parts.Num();
	Submission.ExperienceGained = ExperienceGained;
	Submission.ElapsedSeconds = ReviewResult.ElapsedSeconds;
	Submission.bIsCorrect = ReviewResult.bIsCorrect;
	Submission.Feedback = ReviewResult.Feedback;
	Submission.SubmittedAt = FDateTime::UtcNow();
	for (const FStudyPartResult& Part : ReviewResult.Parts)
	{
		FStudySavedPart SavedPart;
		SavedPart.Label = Part.Label;
		SavedPart.bIsCorrect = Part.bIsCorrect;
		Submission.Parts.Add(SavedPart);
		Submission.CorrectPartCount += Part.bIsCorrect ? 1 : 0;
	}
	return Submission;
}

void UStudyProgressSubsystem::SetBoardImage(
	const FString& ProblemId, const TArray<uint8>& PngBytes)
{
	ProgressSave->Problems.FindOrAdd(ProblemId).BoardImagePngBytes =
		PngBytes;
}

void UStudyProgressSubsystem::StoreExperience(
	const AStudyPlayerState& PlayerState)
{
	ProgressSave->SubjectExperience =
		PlayerState.GetTopicExperience(EStudyTopicKind::Subject);
	ProgressSave->SectionExperience =
		PlayerState.GetTopicExperience(EStudyTopicKind::Section);
	ProgressSave->PartAwardTimes = PlayerState.GetPartAwardTimes();
}

void UStudyProgressSubsystem::RestoreExperience(
	AStudyPlayerState& PlayerState) const
{
	PlayerState.RestoreProgress(ProgressSave->SubjectExperience,
								ProgressSave->SectionExperience,
								ProgressSave->PartAwardTimes);
}

void UStudyProgressSubsystem::SaveToDisk() const
{
	UGameplayStatics::SaveGameToSlot(
		ProgressSave, BuildSlotName(), SaveUserIndex);
}

const TMap<FString, FStudySavedProblem>&
	UStudyProgressSubsystem::GetProblems() const
{
	return ProgressSave->Problems;
}

void UStudyProgressSubsystem::Deinitialize()
{
	SaveToDisk();
	Super::Deinitialize();
}
