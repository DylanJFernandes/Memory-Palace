// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/ProblemSlotComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

UProblemSlotComponent::UProblemSlotComponent()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		PlaneFinder{TEXT("/Engine/BasicShapes/Plane.Plane")};
	PlaneMesh = PlaneFinder.Object;

	/* Arrows are editor only by default, which would also drop the
	   plane from play */
#if WITH_EDITORONLY_DATA
	bIsEditorOnly = false;
	SetIsVisualizationComponent(false);
#endif
}

void UProblemSlotComponent::OnRegister()
{
	Super::OnRegister();

	AActor* Owner{GetOwner()};
	if (!Owner || PreviewPlane)
	{
		return;
	}

	if (!PlaneMesh)
	{
		return;
	}

	PreviewPlane = NewObject<UStaticMeshComponent>(
		Owner, NAME_None, RF_Transient);
	PreviewPlane->SetStaticMesh(PlaneMesh);
	PreviewPlane->SetupAttachment(this);

	/* The plane mesh faces up, so tip it to face the arrow */
	PreviewPlane->SetRelativeRotation(FRotator{-90.0, 0.0, 0.0});
	PreviewPlane->SetRelativeScale3D(FVector{PreviewScale});
	PreviewPlane->SetHiddenInGame(false);
	PreviewPlane->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	PreviewPlane->RegisterComponent();
}

void UProblemSlotComponent::OnUnregister()
{
	if (PreviewPlane)
	{
		PreviewPlane->DestroyComponent();
		PreviewPlane = nullptr;
	}

	Super::OnUnregister();
}
