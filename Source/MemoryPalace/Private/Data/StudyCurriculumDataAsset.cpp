// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/StudyCurriculumDataAsset.h"

FStudySectionDefinition::FStudySectionDefinition()
	: RequiredPrerequisiteExperience{0}
{
}

const TArray<FStudySectionDefinition>&
	UStudyCurriculumDataAsset::GetSections() const
{
	return Sections;
}
