// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/StudyCurriculum.h"
#include "Dom/JsonObject.h"

FStudyCurriculum FStudyCurriculum::FromJson(
	const FJsonObject& JsonObject)
{
	FStudyCurriculum Curriculum;
	Curriculum.ReadSections(JsonObject);
	Curriculum.ReadProblems(JsonObject);
	return Curriculum;
}

void FStudyCurriculum::ReadSections(const FJsonObject& JsonObject)
{
	const TArray<TSharedPtr<FJsonValue>>* SectionValues{nullptr};
	if (!JsonObject.TryGetArrayField(TEXT("sections"), SectionValues))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& SectionValue : *SectionValues)
	{
		const TSharedPtr<FJsonObject> SectionObject{
			SectionValue->AsObject()};
		if (!SectionObject.IsValid())
		{
			continue;
		}
		FStudyCurriculumSection Section;
		Section.SectionId =
			SectionObject->GetStringField(TEXT("sectionId"));
		Section.SectionName =
			SectionObject->GetStringField(TEXT("sectionName"));
		Sections.Add(Section);
	}
}

void FStudyCurriculum::ReadProblems(const FJsonObject& JsonObject)
{
	const TArray<TSharedPtr<FJsonValue>>* ProblemValues{nullptr};
	if (!JsonObject.TryGetArrayField(TEXT("problems"), ProblemValues))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& ProblemValue : *ProblemValues)
	{
		const TSharedPtr<FJsonObject> ProblemObject{
			ProblemValue->AsObject()};
		if (!ProblemObject.IsValid())
		{
			continue;
		}
		const FString SectionId{
			ProblemObject->GetStringField(TEXT("sectionId"))};
		FStudyCurriculumSection* Section{Sections.FindByPredicate(
			[&SectionId](const FStudyCurriculumSection& Candidate)
			{
				return Candidate.SectionId == SectionId;
			})};
		if (Section)
		{
			Section->ProblemIds.Add(
				ProblemObject->GetStringField(TEXT("id")));
		}
	}
}
