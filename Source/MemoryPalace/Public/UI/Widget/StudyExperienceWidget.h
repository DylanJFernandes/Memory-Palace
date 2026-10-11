// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/StudyTopicKind.h"
#include "StudyExperienceWidget.generated.h"

class UProgressBar;
class UTextBlock;

/* Shows the experience bars for the subject and the section */
UCLASS()
class MEMORYPALACE_API UStudyExperienceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Sets default values for this widget */
	UStudyExperienceWidget(
		const FObjectInitializer& ObjectInitializer);

	/* Shows a topic's new total on the bar for its kind */
	void SetExperience(EStudyTopicKind TopicKind, FName TopicName,
					   int32 TotalExperience);

protected:
	/* Shows empty bars until experience is earned */
	virtual void NativeConstruct() override;

private:
	/* Returns the label with the topic, level and progress */
	FText BuildLabel(FName TopicName, int32 TotalExperience) const;

	/* Returns how full the bar is within the current level */
	float ComputeFillPercent(int32 TotalExperience) const;

	/* Experience needed to fill a bar and gain a level */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Experience")
	int32 ExperiencePerLevel;

	/* Bar for the broad subject */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> SubjectProgressBar;

	/* Label for the broad subject */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SubjectLabelTextBlock;

	/* Bar for the section */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> SectionProgressBar;

	/* Label for the section */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SectionLabelTextBlock;
};
