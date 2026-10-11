// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actor/CastleActor.h"
#include "Component/ProblemSlotComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/CastleLayoutDataAsset.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName TintParameterName{TEXT("Tint")};
	const FName ColorParameterName{TEXT("Color")};

	constexpr double ByteRange{255.0};
	constexpr double DoorOffset{-13.0};
	constexpr double DoorScale{0.65};
}

ACastleActor::ACastleActor()
	: CastleLayout{nullptr}
	, DoorClass{nullptr}
	, DoorRelativeTransform{
		  FRotator::ZeroRotator, FVector{DoorOffset, 0.0, 0.0},
		  FVector{DoorScale}}
	, CastleHue{0}
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* RootSceneComponent{
		CreateDefaultSubobject<USceneComponent>(TEXT("Root"))};
	RootSceneComponent->SetMobility(EComponentMobility::Static);
	SetRootComponent(RootSceneComponent);

	StoneTint.Saturation = 0.45f;
	StoneTint.Brightness = 3.1f;
	RoofTint.Saturation = 0.8f;
	RoofTint.Brightness = 2.0f;
	FloorTint.Saturation = 0.35f;
	FloorTint.Brightness = 1.5f;
	GlassTint.Saturation = 0.8f;
	GlassTint.Brightness = 0.9f;
	DoorTint.Saturation = 0.8f;
	DoorTint.Brightness = 1.5f;
}

void ACastleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ClearGeneratedComponents();
	if (!CastleLayout)
	{
		return;
	}
	const FCastleMaterialMap Materials{CreateCastleMaterials()};
	CreatePieces(Materials);
	CreateDoor(Materials);
}

void ACastleActor::ClearGeneratedComponents()
{
	for (UActorComponent* Component : GeneratedComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	GeneratedComponents.Reset();
}

void ACastleActor::CreatePieces(const FCastleMaterialMap& Materials)
{
	for (const FCastlePieceDefinition& Piece :
		 CastleLayout->GetPieces())
	{
		UStaticMeshComponent* PieceComponent{CreateMeshComponent(
			Piece.Mesh, Piece.RelativeTransform)};
		ApplyMaterials(PieceComponent, Materials);
	}
}

UStaticMeshComponent* ACastleActor::CreateMeshComponent(
	UStaticMesh* Mesh, const FTransform& RelativeTransform)
{
	UStaticMeshComponent* MeshComponent{
		NewObject<UStaticMeshComponent>(this)};
	MeshComponent->CreationMethod =
		EComponentCreationMethod::UserConstructionScript;
	MeshComponent->SetMobility(EComponentMobility::Static);
	MeshComponent->SetStaticMesh(Mesh);
	MeshComponent->SetupAttachment(GetRootComponent());
	MeshComponent->SetRelativeTransform(RelativeTransform);
	MeshComponent->RegisterComponent();
	GeneratedComponents.Add(MeshComponent);
	return MeshComponent;
}

void ACastleActor::ApplyMaterials(
	UStaticMeshComponent* MeshComponent,
	const FCastleMaterialMap& Materials)
{
	for (int32 SlotIndex{0};
		 SlotIndex < MeshComponent->GetNumMaterials(); ++SlotIndex)
	{
		UMaterialInstanceDynamic* const* TintedMaterial{
			Materials.Find(MeshComponent->GetMaterial(SlotIndex))};
		if (TintedMaterial)
		{
			MeshComponent->SetMaterial(SlotIndex, *TintedMaterial);
		}
	}
}

void ACastleActor::CreateDoor(const FCastleMaterialMap& Materials)
{
	if (!DoorClass)
	{
		return;
	}
	UChildActorComponent* DoorComponent{
		NewObject<UChildActorComponent>(this)};
	DoorComponent->CreationMethod =
		EComponentCreationMethod::UserConstructionScript;
	DoorComponent->SetChildActorClass(DoorClass);
	DoorComponent->SetupAttachment(GetRootComponent());
	DoorComponent->SetRelativeTransform(DoorRelativeTransform);
	DoorComponent->RegisterComponent();
	GeneratedComponents.Add(DoorComponent);

	TintDoorActor(DoorComponent->GetChildActor(), Materials);
}

void ACastleActor::TintDoorActor(
	AActor* DoorActor, const FCastleMaterialMap& Materials)
{
	if (!DoorActor)
	{
		return;
	}
	TArray<UStaticMeshComponent*> DoorMeshes;
	DoorActor->GetComponents(DoorMeshes);
	for (UStaticMeshComponent* DoorMesh : DoorMeshes)
	{
		ApplyMaterials(DoorMesh, Materials);
	}
}

FCastleMaterialMap ACastleActor::CreateCastleMaterials()
{
	const TArray<const FCastleTintSettings*> AllTintSettings{
		&StoneTint, &RoofTint, &FloorTint, &GlassTint, &DoorTint};

	FCastleMaterialMap Materials;
	for (const FCastleTintSettings* TintSettings : AllTintSettings)
	{
		if (TintSettings->BaseMaterial)
		{
			Materials.Add(TintSettings->BaseMaterial,
						  CreateTintedMaterial(*TintSettings));
		}
	}
	return Materials;
}

UMaterialInstanceDynamic* ACastleActor::CreateTintedMaterial(
	const FCastleTintSettings& TintSettings)
{
	const uint8 Saturation{static_cast<uint8>(
		FMath::RoundToInt(TintSettings.Saturation * ByteRange))};
	const FLinearColor Tint{
		FLinearColor::MakeFromHSV8(CastleHue, Saturation, 255)
		* TintSettings.Brightness};

	UMaterialInstanceDynamic* TintedMaterial{
		UMaterialInstanceDynamic::Create(
			TintSettings.BaseMaterial, this)};
	TintedMaterial->SetVectorParameterValue(TintParameterName, Tint);
	TintedMaterial->SetVectorParameterValue(ColorParameterName, Tint);
	return TintedMaterial;
}

void ACastleActor::SetCastleHue(uint8 InCastleHue)
{
	CastleHue = InCastleHue;
	RerunConstructionScripts();
}

int32 ACastleActor::GetProblemCapacity() const
{
	TArray<UProblemSlotComponent*> ProblemSlots;
	GetComponents(ProblemSlots);
	return ProblemSlots.Num();
}
