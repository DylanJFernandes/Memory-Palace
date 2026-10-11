// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/StudyGameState.h"

AStudyGameState::AStudyGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStudyGameState::UnlockSection(FName SectionName)
{
	if (IsSectionUnlocked(SectionName))
	{
		return;
	}
	UnlockedSectionNames.Add(SectionName);
	OnSectionUnlocked.Broadcast(SectionName);
}

bool AStudyGameState::IsSectionUnlocked(FName SectionName) const
{
	return UnlockedSectionNames.Contains(SectionName);
}

void AStudyGameState::SetActiveProblem(const FString& ProblemId)
{
	ActiveProblemId = ProblemId;
	OnActiveProblemChanged.Broadcast(ActiveProblemId);
}

const FString& AStudyGameState::GetActiveProblemId() const
{
	return ActiveProblemId;
}
