// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CastleTintSettings.generated.h"

class UMaterialInterface;

/* A castle material and how strongly its tint is colored */
USTRUCT()
struct FCastleTintSettings
{
	GENERATED_BODY()

	/* Sets default values for these tint settings */
	FCastleTintSettings();

	/* Material the castle's meshes use for this surface */
	UPROPERTY(EditAnywhere, Category = "Custom|Tint")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/* How vivid the castle's hue is on this surface, 0 to 1 */
	UPROPERTY(EditAnywhere, Category = "Custom|Tint",
			  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Saturation;

	/* How bright the tint is, above 1 brightens the texture */
	UPROPERTY(EditAnywhere, Category = "Custom|Tint",
			  meta = (ClampMin = "0.0"))
	float Brightness;
};
