// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StudyCurriculumDataAsset.generated.h"

/* One section of the course and what it takes to unlock it */
USTRUCT()
struct FStudySectionDefinition
{
	GENERATED_BODY()

	/* Sets default values for this section definition */
	FStudySectionDefinition();

	/* Name of the section, matching the server's sectionName */
	UPROPERTY(EditAnywhere, Category = "Custom|Section")
	FName SectionName;

	/* Section that must be progressed first, or none if this
	   section starts unlocked */
	UPROPERTY(EditAnywhere, Category = "Custom|Section")
	FName PrerequisiteSectionName;

	/* Experience needed in the prerequisite section to unlock
	   this one */
	UPROPERTY(EditAnywhere, Category = "Custom|Section")
	int32 RequiredPrerequisiteExperience;
};

/* Ordered sections of the course, written by hand for now and
   later filled in from the AI's reading of the course material */
UCLASS()
class MEMORYPALACE_API UStudyCurriculumDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/* Returns every section of the course */
	const TArray<FStudySectionDefinition>& GetSections() const;

private:
	/* Sections of the course, in the order they are taught */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Curriculum")
	TArray<FStudySectionDefinition> Sections;
};
