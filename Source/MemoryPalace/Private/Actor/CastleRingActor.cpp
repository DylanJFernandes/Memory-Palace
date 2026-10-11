// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actor/CastleRingActor.h"
#include "Actor/CastleActor.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr double FullCircleDegrees{360.0};
	constexpr double ByteRange{255.0};
	constexpr double PathHeight{-2.0};
	constexpr double UnitCubeSize{100.0};
	constexpr double DefaultCastleSpacing{3218.0};
	constexpr double DefaultMinimumRingRadius{4705.0};
	constexpr double DefaultPathStartRadius{2700.0};
	constexpr double DefaultPathWidth{500.0};
	constexpr double DefaultPathThickness{20.0};
}

ACastleRingActor::ACastleRingActor()
	: CastleCount{9}
	, CastleSpacing{DefaultCastleSpacing}
	, MinimumRingRadius{DefaultMinimumRingRadius}
	, HueOffsetDegrees{0.0}
	, CastleClass{nullptr}
	, PathMesh{nullptr}
	, PathMaterial{nullptr}
	, PathStartRadius{DefaultPathStartRadius}
	, PathWidth{DefaultPathWidth}
	, PathThickness{DefaultPathThickness}
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* RootSceneComponent{
		CreateDefaultSubobject<USceneComponent>(TEXT("Root"))};
	RootSceneComponent->SetMobility(EComponentMobility::Static);
	SetRootComponent(RootSceneComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		CubeFinder{TEXT("/Engine/BasicShapes/Cube.Cube")};
	PathMesh = CubeFinder.Object;
}

void ACastleRingActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ClearGeneratedComponents();
	for (int32 CastleIndex{0}; CastleIndex < CastleCount;
		 ++CastleIndex)
	{
		if (CastleClass)
		{
			CreateCastleComponent(CastleIndex);
		}
		BuildPath(CastleIndex);
	}
}

void ACastleRingActor::ClearGeneratedComponents()
{
	for (UActorComponent* Component : GeneratedComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	GeneratedComponents.Reset();

	/* The list is transient, so components saved in the level or
	   copied into play are found by how they were created */
	TArray<UActorComponent*> StaleComponents;
	GetComponents(StaleComponents);
	for (UActorComponent* Component : StaleComponents)
	{
		if (Component->CreationMethod ==
			EComponentCreationMethod::UserConstructionScript)
		{
			Component->DestroyComponent();
		}
	}
}

bool ACastleRingActor::FindCastleHue(
	const USceneComponent* CastleComponent, uint8& OutHue) const
{
	if (!CastleComponent || CastleCount < 1)
	{
		return false;
	}
	/* Read back from the castle's angle, which survives play and
	   saving, unlike a lookup table kept in memory */
	const double Fraction{FMath::Frac(
		CastleComponent->GetRelativeRotation().Yaw
		/ FullCircleDegrees)};
	const int32 CastleIndex{
		FMath::RoundToInt(Fraction * CastleCount) % CastleCount};
	OutHue = GetCastleHue(CastleIndex);
	return true;
}

UChildActorComponent* ACastleRingActor::CreateCastleComponent(
	int32 CastleIndex)
{
	const FRotator Rotation{
		0.0, GetCastleAngleDegrees(CastleIndex), 0.0};
	const FVector Location{
		Rotation.RotateVector(FVector::ForwardVector)
		* GetRingRadius()};

	UChildActorComponent* CastleComponent{
		NewObject<UChildActorComponent>(this)};
	CastleComponent->CreationMethod =
		EComponentCreationMethod::UserConstructionScript;
	CastleComponent->SetChildActorClass(CastleClass);

	/* Static like the castle's root, or the castle cannot attach */
	CastleComponent->SetMobility(EComponentMobility::Static);

	CastleComponent->SetupAttachment(GetRootComponent());
	CastleComponent->SetRelativeLocationAndRotation(
		Location, Rotation);
	CastleComponent->RegisterComponent();
	GeneratedComponents.Add(CastleComponent);
	return CastleComponent;
}

void ACastleRingActor::BuildPath(int32 CastleIndex)
{
	const double RingRadius{GetRingRadius()};
	const double PathLength{RingRadius - PathStartRadius};
	const double PathCenterRadius{PathStartRadius + PathLength / 2.0};
	const FRotator Rotation{
		0.0, GetCastleAngleDegrees(CastleIndex), 0.0};
	const FVector Location{
		Rotation.RotateVector(FVector::ForwardVector)
		* PathCenterRadius};
	const FVector Scale{PathLength / UnitCubeSize,
						PathWidth / UnitCubeSize,
						PathThickness / UnitCubeSize};

	UStaticMeshComponent* PathComponent{CreateMeshComponent(
		PathMesh,
		FTransform{Rotation, Location + FVector{0, 0, PathHeight},
				   Scale})};
	PathComponent->SetMaterial(0, PathMaterial);
}

UStaticMeshComponent* ACastleRingActor::CreateMeshComponent(
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

uint8 ACastleRingActor::GetCastleHue(int32 CastleIndex) const
{
	const double CastleFraction{
		static_cast<double>(CastleIndex) / CastleCount};
	const double OffsetFraction{
		HueOffsetDegrees / FullCircleDegrees};
	return static_cast<uint8>(
		FMath::Frac(CastleFraction + OffsetFraction) * ByteRange);
}

double ACastleRingActor::GetCastleAngleDegrees(
	int32 CastleIndex) const
{
	return FullCircleDegrees * CastleIndex / CastleCount;
}

double ACastleRingActor::GetRingRadius() const
{
	if (CastleCount < 2)
	{
		return MinimumRingRadius;
	}
	const double HalfStepRadians{UE_DOUBLE_PI / CastleCount};
	const double SpacedRadius{
		CastleSpacing / (2.0 * FMath::Sin(HalfStepRadians))};
	return FMath::Max(MinimumRingRadius, SpacedRadius);
}
