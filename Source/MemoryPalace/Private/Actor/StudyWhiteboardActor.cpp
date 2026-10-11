// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actor/StudyWhiteboardActor.h"
#include "Component/StudyWhiteboardComponent.h"
#include "Components/StaticMeshComponent.h"

AStudyWhiteboardActor::AStudyWhiteboardActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WhiteboardMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("WhiteboardMesh"));
	SetRootComponent(WhiteboardMesh);

	WhiteboardComponent =
		CreateDefaultSubobject<UStudyWhiteboardComponent>(
			TEXT("WhiteboardComponent"));
}
