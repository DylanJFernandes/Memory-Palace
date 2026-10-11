// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "ProblemSlotComponent.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/* Marks a spot in a castle where a problem can be placed, and
   which way it faces */
UCLASS(meta = (BlueprintSpawnableComponent))
class MEMORYPALACE_API UProblemSlotComponent : public UArrowComponent
{
	GENERATED_BODY()

public:
	/* Finds the plane mesh used for the preview */
	UProblemSlotComponent();

protected:
	/* Adds the preview plane */
	virtual void OnRegister() override;

	/* Removes the preview plane */
	virtual void OnUnregister() override;

private:
	/* Scale of the preview plane, 1 is 100 units across */
	UPROPERTY(EditAnywhere, Category = "Problem Slot")
	float PreviewScale{2.5f};

	/* Mesh of the preview plane */
	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh;

	/* Plane that shows how big a problem would look here */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PreviewPlane;
};
