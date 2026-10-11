// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StudyProblemDataAsset.generated.h"

/* Game-side rewards and links for one problem the server owns */
UCLASS()
class MEMORYPALACE_API UStudyProblemDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/* Sets default values for this data asset */
	UStudyProblemDataAsset();

	/* Returns the id of the problem on the study pad server */
	const FString& GetProblemId() const;

	/* Returns the broad topic that gains experience, such as
	   Precalculus */
	FName GetSubjectTopicName() const;

	/* Returns the specific topic that gains experience, such as
	   Quadratic Functions */
	FName GetSectionTopicName() const;

	/* Returns the experience given for a correct answer */
	int32 GetCorrectAnswerExperience() const;

	/* Returns the section a correct answer unlocks, or none */
	FName GetUnlockedSectionName() const;

private:
	/* Id of the problem, matching an id in the server's
	   problems.json */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Problem")
	FString ProblemId;

	/* Broad topic that gains experience, such as Precalculus */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Experience")
	FName SubjectTopicName;

	/* Specific topic that gains experience, such as Quadratic
	   Functions */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Experience")
	FName SectionTopicName;

	/* Experience given to both topics for a correct answer */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Experience")
	int32 CorrectAnswerExperience;

	/* Section a correct answer unlocks, or none to unlock
	   nothing */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Progression")
	FName UnlockedSectionName;
};
