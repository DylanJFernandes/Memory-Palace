// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CastleRingActor.generated.h"

class ACastleActor;
class UChildActorComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/* Builds a ring of castles around the center, each joined to the
   center by a path and given its own color */
UCLASS()
class MEMORYPALACE_API ACastleRingActor : public AActor
{
	GENERATED_BODY()

public:
	/* Sets default values for this ring */
	ACastleRingActor();

	/* Works out the hue of the castle that stands in a child actor
	   component of this ring from its angle */
	bool FindCastleHue(const USceneComponent* CastleComponent,
					   uint8& OutHue) const;

	/* Rebuilds every castle and path from the current settings */
	virtual void OnConstruction(
		const FTransform& Transform) override;

private:
	/* Destroys the castles and paths built last time */
	void ClearGeneratedComponents();

	/* Spawns the castle actor of one castle around the center */
	UChildActorComponent* CreateCastleComponent(int32 CastleIndex);

	/* Builds the path that joins one castle to the center */
	void BuildPath(int32 CastleIndex);

	/* Creates a registered mesh component under the root */
	UStaticMeshComponent* CreateMeshComponent(
		UStaticMesh* Mesh, const FTransform& RelativeTransform);

	/* Returns the hue of a castle, spread evenly around the
	   color wheel */
	uint8 GetCastleHue(int32 CastleIndex) const;

	/* Returns the angle a castle sits at around the center */
	double GetCastleAngleDegrees(int32 CastleIndex) const;

	/* Returns the distance from the center to the castles */
	double GetRingRadius() const;

	/* Number of castles placed around the center */
	UPROPERTY(EditAnywhere, Category = "Custom|Ring",
			  meta = (ClampMin = "1", ClampMax = "32"))
	int32 CastleCount;

	/* Closest two neighboring castles are allowed to be */
	UPROPERTY(EditAnywhere, Category = "Custom|Ring")
	double CastleSpacing;

	/* Smallest distance from the center to the castles */
	UPROPERTY(EditAnywhere, Category = "Custom|Ring")
	double MinimumRingRadius;

	/* Degrees the first castle's hue is turned around the
	   color wheel */
	UPROPERTY(EditAnywhere, Category = "Custom|Ring")
	double HueOffsetDegrees;

	/* Castle actor spawned for every castle in the ring */
	UPROPERTY(EditAnywhere, Category = "Custom|Castle")
	TSubclassOf<ACastleActor> CastleClass;

	/* Mesh stretched into each path, one unit cube */
	UPROPERTY(EditAnywhere, Category = "Custom|Path")
	TObjectPtr<UStaticMesh> PathMesh;

	/* Material of the paths */
	UPROPERTY(EditAnywhere, Category = "Custom|Path")
	TObjectPtr<UMaterialInterface> PathMaterial;

	/* Distance from the center where the paths begin */
	UPROPERTY(EditAnywhere, Category = "Custom|Path")
	double PathStartRadius;

	/* Width of each path */
	UPROPERTY(EditAnywhere, Category = "Custom|Path")
	double PathWidth;

	/* Thickness of each path */
	UPROPERTY(EditAnywhere, Category = "Custom|Path")
	double PathThickness;

	/* Castles and paths built by the last rebuild */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> GeneratedComponents;
};
