// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actor/CastleRingActor.h"
#include "Actor/CastleActor.h"
#include "Component/StudyCastleComponent.h"
#include "Engine/World.h"
#include "Game/StudyPadSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr double FullCircleDegrees{360.0};
	constexpr double HueByteRange{255.0};
	constexpr double PathHeight{-2.0};
	constexpr double UnitCubeSize{100.0};
	constexpr double DefaultCastleSpacing{3218.0};
	constexpr double DefaultMinimumRingRadius{4705.0};
	constexpr double DefaultPathStartRadius{2700.0};
	constexpr double DefaultPathWidth{500.0};
	constexpr double DefaultPathThickness{20.0};
	const FName OrbColorParameterName{TEXT("OrbColor")};
	const FName GlowColorParameterName{TEXT("GlowColor")};
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
	, CompletedOrbColor{FLinearColor::Green}
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

void ACastleRingActor::BeginPlay()
{
	Super::BeginPlay();
	UStudyPadSubsystem* StudyPadSubsystem{
		GetWorld()->GetSubsystem<UStudyPadSubsystem>()};
	if (!StudyPadSubsystem)
	{
		return;
	}
	StudyPadSubsystem->OnCurriculumLoaded.AddUObject(
		this, &ACastleRingActor::HandleCurriculumLoaded);
	StudyPadSubsystem->RequestCurriculum();
}

void ACastleRingActor::HandleCurriculumLoaded(
	const FStudyCurriculum& Curriculum)
{
	if (AssignedCastles.Num() > 0)
	{
		return;
	}
	const TArray<UStudyCastleComponent*> Castles{
		GetCastlesInRingOrder()};
	const int32 AssignedCount{
		FMath::Min(Castles.Num(), Curriculum.Sections.Num())};
	for (int32 CastleIndex{0}; CastleIndex < AssignedCount;
		 ++CastleIndex)
	{
		UStudyCastleComponent* Castle{Castles[CastleIndex]};
		Castle->AssignSection(Curriculum.Sections[CastleIndex],
							  GetOrbTargetLocation());
		Castle->OnCastleCompleted.BindUObject(
			this, &ACastleRingActor::HandleCastleCompleted);
		AssignedCastles.Add(Castle);
	}
}

TArray<UStudyCastleComponent*>
	ACastleRingActor::GetCastlesInRingOrder() const
{
	TArray<UChildActorComponent*> CastleComponents;
	GetComponents(CastleComponents);
	CastleComponents.Sort(
		[this](const UChildActorComponent& Left,
			   const UChildActorComponent& Right)
		{
			return GetCastleIndex(&Left) < GetCastleIndex(&Right);
		});
	TArray<UStudyCastleComponent*> Castles;
	for (const UChildActorComponent* CastleComponent :
		 CastleComponents)
	{
		const AActor* Castle{CastleComponent->GetChildActor()};
		UStudyCastleComponent* StudyCastle{Castle
			? Castle->FindComponentByClass<UStudyCastleComponent>()
			: nullptr};
		if (StudyCastle)
		{
			Castles.Add(StudyCastle);
		}
	}
	return Castles;
}

int32 ACastleRingActor::GetCastleIndex(
	const USceneComponent* CastleComponent) const
{
	const double Fraction{FMath::Frac(
		CastleComponent->GetRelativeRotation().Yaw
		/ FullCircleDegrees)};
	return FMath::RoundToInt(Fraction * CastleCount) % CastleCount;
}

FVector ACastleRingActor::GetOrbTargetLocation() const
{
	const AActor* CenterOrb{CenterOrbActor.Get()};
	return CenterOrb ? CenterOrb->GetActorLocation()
					 : GetActorLocation();
}

void ACastleRingActor::HandleCastleCompleted()
{
	if (AreAllCastlesCompleted())
	{
		MakeCenterOrbGlowGreen();
	}
}

bool ACastleRingActor::AreAllCastlesCompleted() const
{
	for (const TWeakObjectPtr<UStudyCastleComponent>& Castle :
		 AssignedCastles)
	{
		if (Castle.IsValid() && !Castle->IsCompleted())
		{
			return false;
		}
	}
	return true;
}

void ACastleRingActor::MakeCenterOrbGlowGreen()
{
	AActor* CenterOrb{CenterOrbActor.Get()};
	if (!CenterOrb)
	{
		return;
	}
	TArray<UNiagaraComponent*> ParticleComponents;
	CenterOrb->GetComponents(ParticleComponents);
	for (UNiagaraComponent* ParticleComponent : ParticleComponents)
	{
		ParticleComponent->SetVariableLinearColor(
			OrbColorParameterName, CompletedOrbColor);
	}
	TArray<UStaticMeshComponent*> MeshComponents;
	CenterOrb->GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		UMaterialInstanceDynamic* Material{
			MeshComponent->CreateDynamicMaterialInstance(0)};
		if (Material)
		{
			Material->SetVectorParameterValue(
				GlowColorParameterName, CompletedOrbColor);
		}
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
		FMath::Frac(CastleFraction + OffsetFraction) * HueByteRange);
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
