// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CastleLayoutDataAsset.generated.h"

class UStaticMesh;

/* One mesh of a castle and where it sits inside that castle */
USTRUCT()
struct FCastlePieceDefinition
{
	GENERATED_BODY()

	/* Sets default values for this piece definition */
	FCastlePieceDefinition();

	/* Mesh that makes up this piece of the castle */
	UPROPERTY(EditAnywhere, Category = "Custom|Piece")
	TObjectPtr<UStaticMesh> Mesh;

	/* Transform of the piece relative to the castle's center */
	UPROPERTY(EditAnywhere, Category = "Custom|Piece")
	FTransform RelativeTransform;
};

/* Every mesh that makes up one castle, shared by all castles */
UCLASS()
class MEMORYPALACE_API UCastleLayoutDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/* Returns every piece of a single castle */
	const TArray<FCastlePieceDefinition>& GetPieces() const;

private:
	/* Pieces of a single castle, relative to its center */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Castle")
	TArray<FCastlePieceDefinition> Pieces;
};
