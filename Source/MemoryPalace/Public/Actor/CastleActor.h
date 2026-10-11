// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/CastleTintSettings.h"
#include "GameFramework/Actor.h"
#include "CastleActor.generated.h"

class UCastleLayoutDataAsset;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/* Maps each base castle material to this castle's tinted copy */
using FCastleMaterialMap = TMap<UMaterialInterface*,
								UMaterialInstanceDynamic*>;

/* One tinted castle with a fixed number of problem slots placed
   in its Blueprint */
UCLASS()
class MEMORYPALACE_API ACastleActor : public AActor
{
	GENERATED_BODY()

public:
	/* Sets default values for this castle */
	ACastleActor();

	/* Returns how many problems this castle can hold */
	int32 GetProblemCapacity() const;

	/* Rebuilds the castle from the current settings */
	virtual void OnConstruction(
		const FTransform& Transform) override;

private:
	/* Clears and recreates the pieces and door */
	void RebuildCastle();

	/* Destroys the pieces and door built last time */
	void ClearGeneratedComponents();

	/* Creates every mesh of the castle */
	void CreatePieces(const FCastleMaterialMap& Materials);

	/* Creates the door in the castle's doorway, tinted to match */
	void CreateDoor(const FCastleMaterialMap& Materials);

	/* Swaps the base materials on every mesh of a door */
	void TintDoorActor(AActor* DoorActor,
					   const FCastleMaterialMap& Materials);

	/* Creates a registered mesh component under the root */
	UStaticMeshComponent* CreateMeshComponent(
		UStaticMesh* Mesh, const FTransform& RelativeTransform);

	/* Swaps each base material on a mesh for its tinted copy */
	void ApplyMaterials(UStaticMeshComponent* MeshComponent,
						const FCastleMaterialMap& Materials);

	/* Returns the hue given by the ring this castle stands in, or
	   this castle's own hue when it stands alone */
	uint8 ResolveCastleHue() const;

	/* Creates the tinted material copies for this castle */
	FCastleMaterialMap CreateCastleMaterials(uint8 Hue);

	/* Creates a copy of a base material tinted with a hue */
	UMaterialInstanceDynamic* CreateTintedMaterial(
		const FCastleTintSettings& TintSettings, uint8 Hue);

	/* Pieces that make up the castle */
	UPROPERTY(EditAnywhere, Category = "Custom|Castle")
	TObjectPtr<UCastleLayoutDataAsset> CastleLayout;

	/* Stone walls, towers and arches */
	UPROPERTY(EditAnywhere, Category = "Custom|Castle")
	FCastleTintSettings StoneTint;

	/* Roofs and tower tops */
	UPROPERTY(EditAnywhere, Category = "Custom|Castle")
	FCastleTintSettings RoofTint;

	/* Castle floors */
	UPROPERTY(EditAnywhere, Category = "Custom|Castle")
	FCastleTintSettings FloorTint;

	/* Stained glass windows */
	UPROPERTY(EditAnywhere, Category = "Custom|Castle")
	FCastleTintSettings GlassTint;

	/* Door wood */
	UPROPERTY(EditAnywhere, Category = "Custom|Door")
	FCastleTintSettings DoorTint;

	/* Door actor placed in the castle's doorway */
	UPROPERTY(EditAnywhere, Category = "Custom|Door")
	TSubclassOf<AActor> DoorClass;

	/* Where the door sits relative to the castle's center */
	UPROPERTY(EditAnywhere, Category = "Custom|Door")
	FTransform DoorRelativeTransform;

	/* Hue on the color wheel, 0 to 255, used when the castle is
	   placed alone. A castle in a ring takes its hue from the ring */
	UPROPERTY(EditAnywhere, Category = "Custom|Castle",
			  meta = (ClampMin = "0", ClampMax = "255"))
	uint8 CastleHue;

	/* Pieces and door built by the last rebuild */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> GeneratedComponents;
};
