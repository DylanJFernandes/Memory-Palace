// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "StudyProgressSaveGame.generated.h"

/* Saved verdict for one part of a submitted problem */
USTRUCT()
struct FStudySavedPart
{
	GENERATED_BODY()

	/* Sets default values for this saved part */
	FStudySavedPart();

	/* Name of the part shown to the student */
	UPROPERTY(SaveGame)
	FString Label;

	/* Whether the student's answer for this part was correct */
	UPROPERTY(SaveGame)
	bool bIsCorrect;
};

/* Saved outcome of one submission of a problem */
USTRUCT()
struct FStudySavedSubmission
{
	GENERATED_BODY()

	/* Sets default values for this saved submission */
	FStudySavedSubmission();

	/* Number of parts that were correct */
	UPROPERTY(SaveGame)
	int32 CorrectPartCount;

	/* Number of parts the problem has */
	UPROPERTY(SaveGame)
	int32 PartCount;

	/* Experience this submission earned */
	UPROPERTY(SaveGame)
	int32 ExperienceGained;

	/* Seconds spent on the problem when it was submitted */
	UPROPERTY(SaveGame)
	double ElapsedSeconds;

	/* Whether every part was correct */
	UPROPERTY(SaveGame)
	bool bIsCorrect;

	/* Plain text feedback the AI gave */
	UPROPERTY(SaveGame)
	FString Feedback;

	/* When the submission was graded */
	UPROPERTY(SaveGame)
	FDateTime SubmittedAt;

	/* Verdict for each part of the problem */
	UPROPERTY(SaveGame)
	TArray<FStudySavedPart> Parts;
};

/* Everything saved about one problem */
USTRUCT()
struct FStudySavedProblem
{
	GENERATED_BODY()

	/* Sets default values for this saved problem */
	FStudySavedProblem();

	/* Broad subject of the problem, such as Precalculus */
	UPROPERTY(SaveGame)
	FString SubjectName;

	/* Readable name of the section the problem belongs to */
	UPROPERTY(SaveGame)
	FString SectionName;

	/* Whether any submission had every part correct */
	UPROPERTY(SaveGame)
	bool bIsSolved;

	/* PNG of the whiteboard as the student last left it */
	UPROPERTY(SaveGame)
	TArray<uint8> BoardImagePngBytes;

	/* Every submission of the problem, oldest first */
	UPROPERTY(SaveGame)
	TArray<FStudySavedSubmission> Submissions;
};

/* Save slot contents of one study world */
UCLASS()
class MEMORYPALACE_API UStudyProgressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/* Saved data of each problem, by problem id */
	UPROPERTY(SaveGame)
	TMap<FString, FStudySavedProblem> Problems;

	/* Experience earned in each subject */
	UPROPERTY(SaveGame)
	TMap<FName, int32> SubjectExperience;

	/* Experience earned in each section */
	UPROPERTY(SaveGame)
	TMap<FName, int32> SectionExperience;

	/* When each part of each problem last earned experience */
	UPROPERTY(SaveGame)
	TMap<FString, FDateTime> PartAwardTimes;
};
