// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/* One section of the course; each section is one castle */
struct FStudyCurriculumSection
{
	/* Identifier of the section */
	FString SectionId;

	/* Readable name shown above the section's castle */
	FString SectionName;

	/* Ids of the section's problems, in homework order */
	TArray<FString> ProblemIds;
};

/* The course built by the server from the uploaded problems */
struct FStudyCurriculum
{
	/* Builds the course from the server's curriculum.json */
	static FStudyCurriculum FromJson(const FJsonObject& JsonObject);

	/* Sections in the order they unlock */
	TArray<FStudyCurriculumSection> Sections;

private:
	/* Reads the sections, without their problems */
	void ReadSections(const FJsonObject& JsonObject);

	/* Files every problem under its section, keeping order */
	void ReadProblems(const FJsonObject& JsonObject);
};
