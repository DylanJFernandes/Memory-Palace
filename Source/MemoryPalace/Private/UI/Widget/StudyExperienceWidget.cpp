// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/StudyExperienceWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

UStudyExperienceWidget::UStudyExperienceWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ExperiencePerLevel{500}
{
}

void UStudyExperienceWidget::SetExperience(
	EStudyTopicKind TopicKind, FName TopicName,
	int32 TotalExperience)
{
	const FText Label{BuildLabel(TopicName, TotalExperience)};
	const float FillPercent{ComputeFillPercent(TotalExperience)};
	if (TopicKind == EStudyTopicKind::Subject)
	{
		SubjectLabelTextBlock->SetText(Label);
		SubjectProgressBar->SetPercent(FillPercent);
		return;
	}
	SectionLabelTextBlock->SetText(Label);
	SectionProgressBar->SetPercent(FillPercent);
}

FText UStudyExperienceWidget::BuildLabel(
	FName TopicName, int32 TotalExperience) const
{
	const int32 Level{TotalExperience / ExperiencePerLevel};
	const int32 LevelProgress{TotalExperience % ExperiencePerLevel};
	return FText::FromString(FString::Printf(
		TEXT("%s  Lv %d  (%d / %d XP)"), *TopicName.ToString(),
		Level + 1, LevelProgress, ExperiencePerLevel));
}

float UStudyExperienceWidget::ComputeFillPercent(
	int32 TotalExperience) const
{
	const int32 LevelProgress{TotalExperience % ExperiencePerLevel};
	return static_cast<float>(LevelProgress) / ExperiencePerLevel;
}
