// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/CastleLayoutDataAsset.h"

FCastlePieceDefinition::FCastlePieceDefinition()
	: Mesh{nullptr}
	, RelativeTransform{FTransform::Identity}
{
}

const TArray<FCastlePieceDefinition>&
	UCastleLayoutDataAsset::GetPieces() const
{
	return Pieces;
}
