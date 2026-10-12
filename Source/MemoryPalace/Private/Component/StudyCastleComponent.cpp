// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/StudyCastleComponent.h"
#include "Actor/StudyWhiteboardActor.h"
#include "Component/ProblemSlotComponent.h"
#include "Component/StudyWhiteboardComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Game/StudyGameState.h"
#include "Game/StudyPadSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	constexpr double SlotFullCircleDegrees{360.0};
	constexpr double SecondsPerDay{86400.0};
	const FName OrbTargetParameterName{TEXT("TargetLocation")};

	/* Castles face the ring's center, so the player walks in
	   along the castle's forward axis */
	const FVector WalkInDirection{FVector::ForwardVector};
}

UStudyCastleComponent::UStudyCastleComponent()
	: WhiteboardClass{nullptr}
	, ExperienceOrbSystem{nullptr}
	, BeaconSystem{nullptr}
	, SolvedSound{nullptr}
	, BeaconSound{nullptr}
	, UnlockSound{nullptr}
	, CrackParameterName{TEXT("CrackAmount")}
	, RetentionHalfLifeDays{30.0}
	, LabelOffset{-300.0, 0.0, 400.0}
	, LabelTextSize{150.0f}
	, OrbTargetLocation{FVector::ZeroVector}
	, bIsUnlocked{false}
	, bHasFiredBeacon{false}
	, LabelComponent{nullptr}
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudyCastleComponent::AssignSection(
	const FStudyCurriculumSection& InSection,
	const FVector& InOrbTargetLocation)
{
	Section = InSection;
	OrbTargetLocation = InOrbTargetLocation;
	SpawnWhiteboards();
	CreateLabel();
	BindToStudyEvents();
	UpdateDecay();
}

bool UStudyCastleComponent::IsCompleted() const
{
	for (const FString& ProblemId : Section.ProblemIds)
	{
		if (!SolveTimesByProblemId.Contains(ProblemId))
		{
			return false;
		}
	}
	return Section.ProblemIds.Num() > 0;
}

void UStudyCastleComponent::SpawnWhiteboards()
{
	TArray<UProblemSlotComponent*> ProblemSlots{
		GetSlotsInEntranceOrder()};
	for (UProblemSlotComponent* ProblemSlot : ProblemSlots)
	{
		ProblemSlot->SetVisibility(false, true);
	}
	const int32 BoardCount{FMath::Min(
		ProblemSlots.Num(), Section.ProblemIds.Num())};
	for (int32 BoardIndex{0}; BoardIndex < BoardCount; ++BoardIndex)
	{
		const FString& ProblemId{Section.ProblemIds[BoardIndex]};
		WhiteboardsByProblemId.Add(ProblemId, SpawnWhiteboard(
			ProblemId, ProblemSlots[BoardIndex]));
	}
}

TArray<UProblemSlotComponent*>
	UStudyCastleComponent::GetSlotsInEntranceOrder() const
{
	TArray<UProblemSlotComponent*> ProblemSlots;
	GetOwner()->GetComponents(ProblemSlots);
	const FVector SlotCenter{FindSlotCenter(ProblemSlots)};
	ProblemSlots.Sort(
		[this, &SlotCenter](const UProblemSlotComponent& Left,
							const UProblemSlotComponent& Right)
		{
			return GetAngleFromEntrance(&Left, SlotCenter)
				< GetAngleFromEntrance(&Right, SlotCenter);
		});
	return ProblemSlots;
}

FVector UStudyCastleComponent::FindSlotCenter(
	const TArray<UProblemSlotComponent*>& ProblemSlots) const
{
	FVector LocationSum{FVector::ZeroVector};
	for (const UProblemSlotComponent* ProblemSlot : ProblemSlots)
	{
		LocationSum += GetLocalLocation(ProblemSlot);
	}
	return ProblemSlots.Num() > 0 ? LocationSum / ProblemSlots.Num()
								  : FVector::ZeroVector;
}

FVector UStudyCastleComponent::GetLocalLocation(
	const UProblemSlotComponent* ProblemSlot) const
{
	return GetOwner()->GetActorTransform().InverseTransformPosition(
		ProblemSlot->GetComponentLocation());
}

double UStudyCastleComponent::GetAngleFromEntrance(
	const UProblemSlotComponent* ProblemSlot,
	const FVector& SlotCenter) const
{
	const FVector SlotDirection{
		(GetLocalLocation(ProblemSlot) - SlotCenter)
			.GetSafeNormal2D()};
	const FVector LeftDirection{
		FRotator{0.0, -90.0, 0.0}.RotateVector(WalkInDirection)};
	const double AngleDegrees{FMath::RadiansToDegrees(FMath::Atan2(
		FVector::DotProduct(SlotDirection, LeftDirection),
		FVector::DotProduct(SlotDirection, -WalkInDirection)))};
	return AngleDegrees < 0.0
		? AngleDegrees + SlotFullCircleDegrees : AngleDegrees;
}

AStudyWhiteboardActor* UStudyCastleComponent::SpawnWhiteboard(
	const FString& ProblemId, UProblemSlotComponent* ProblemSlot)
{
	if (!WhiteboardClass)
	{
		return nullptr;
	}
	AStudyWhiteboardActor* Whiteboard{
		GetWorld()->SpawnActorDeferred<AStudyWhiteboardActor>(
			WhiteboardClass, ProblemSlot->GetComponentTransform(),
			GetOwner())};
	UStudyWhiteboardComponent* WhiteboardComponent{
		Whiteboard
			->FindComponentByClass<UStudyWhiteboardComponent>()};
	if (WhiteboardComponent)
	{
		WhiteboardComponent->SetProblemId(ProblemId);
	}
	Whiteboard->FinishSpawning(ProblemSlot->GetComponentTransform());
	return Whiteboard;
}

void UStudyCastleComponent::CreateLabel()
{
	LabelComponent = NewObject<UTextRenderComponent>(GetOwner());
	LabelComponent->SetMobility(EComponentMobility::Movable);
	LabelComponent->SetupAttachment(GetOwner()->GetRootComponent());
	LabelComponent->RegisterComponent();
	LabelComponent->SetRelativeLocationAndRotation(
		LabelOffset, (-WalkInDirection).Rotation());
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
	LabelComponent->SetWorldSize(LabelTextSize);
	LabelComponent->SetText(FText::FromString(Section.SectionName));
}

void UStudyCastleComponent::BindToStudyEvents()
{
	AStudyGameState* StudyGameState{GetStudyGameState()};
	if (StudyGameState)
	{
		StudyGameState->OnSectionUnlocked.AddUObject(
			this, &UStudyCastleComponent::HandleSectionUnlocked);
		StudyGameState->OnStudyTimeSkipped.AddUObject(
			this, &UStudyCastleComponent::UpdateDecay);
		if (StudyGameState->IsSectionUnlocked(
				FName{Section.SectionName}))
		{
			HandleSectionUnlocked(FName{Section.SectionName});
		}
	}
	UStudyPadSubsystem* StudyPadSubsystem{
		GetWorld()->GetSubsystem<UStudyPadSubsystem>()};
	if (StudyPadSubsystem)
	{
		StudyPadSubsystem->OnReviewFinished.AddUObject(
			this, &UStudyCastleComponent::HandleReviewFinished);
	}
}

void UStudyCastleComponent::HandleSectionUnlocked(
	FName UnlockedSectionName)
{
	if (bIsUnlocked || UnlockedSectionName != Section.SectionName)
	{
		return;
	}
	bIsUnlocked = true;
	DropDoor();
	PlaySoundAtCastle(UnlockSound);
	UpdateDecay();
}

void UStudyCastleComponent::DropDoor()
{
	TArray<UChildActorComponent*> ChildActorComponents;
	GetOwner()->GetComponents(ChildActorComponents);
	for (UChildActorComponent* ChildActorComponent :
		 ChildActorComponents)
	{
		AActor* DoorActor{ChildActorComponent->GetChildActor()};
		if (!DoorActor)
		{
			continue;
		}
		TArray<UStaticMeshComponent*> DoorMeshes;
		DoorActor->GetComponents(DoorMeshes);
		for (UStaticMeshComponent* DoorMesh : DoorMeshes)
		{
			DoorMesh->SetMobility(EComponentMobility::Movable);
			DoorMesh->SetCollisionEnabled(
				ECollisionEnabled::QueryAndPhysics);
			DoorMesh->SetSimulatePhysics(true);
		}
	}
}

void UStudyCastleComponent::HandleReviewFinished(
	const FStudyReviewResult& ReviewResult)
{
	const bool bIsOwnSolvedProblem{
		ReviewResult.bIsCorrect
		&& Section.ProblemIds.Contains(ReviewResult.ProblemId)};
	if (!bIsOwnSolvedProblem)
	{
		return;
	}
	const AStudyGameState* StudyGameState{GetStudyGameState()};
	SolveTimesByProblemId.Add(ReviewResult.ProblemId,
		StudyGameState ? StudyGameState->GetStudyNow()
					   : FDateTime::UtcNow());
	SpawnExperienceOrbs(ReviewResult.ProblemId);
	PlaySoundAtCastle(SolvedSound);
	UpdateDecay();
	if (IsCompleted() && !bHasFiredBeacon)
	{
		CompleteCastle();
	}
}

void UStudyCastleComponent::SpawnExperienceOrbs(
	const FString& ProblemId)
{
	const TWeakObjectPtr<AStudyWhiteboardActor>* Whiteboard{
		WhiteboardsByProblemId.Find(ProblemId)};
	if (!ExperienceOrbSystem || !Whiteboard || !Whiteboard->IsValid())
	{
		return;
	}
	UNiagaraComponent* OrbComponent{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ExperienceOrbSystem,
			(*Whiteboard)->GetActorLocation())};
	if (OrbComponent)
	{
		OrbComponent->SetVariableVec3(
			OrbTargetParameterName, OrbTargetLocation);
	}
}

void UStudyCastleComponent::CompleteCastle()
{
	bHasFiredBeacon = true;
	if (BeaconSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			BeaconSystem, GetOwner()->GetRootComponent(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset, false);
	}
	PlaySoundAtCastle(BeaconSound);
	OnCastleCompleted.ExecuteIfBound();
}

void UStudyCastleComponent::UpdateDecay()
{
	const double Mastery{bIsUnlocked ? ComputeMastery() : 0.0};
	SetCastleMaterialScalar(
		CrackParameterName, static_cast<float>(1.0 - Mastery));
}

double UStudyCastleComponent::ComputeMastery() const
{
	if (Section.ProblemIds.Num() == 0)
	{
		return 0.0;
	}
	double TotalRetention{0.0};
	for (const TPair<FString, FDateTime>& SolveTime :
		 SolveTimesByProblemId)
	{
		TotalRetention += ComputeRetention(SolveTime.Value);
	}
	return TotalRetention / Section.ProblemIds.Num();
}

double UStudyCastleComponent::ComputeRetention(
	const FDateTime& SolveTime) const
{
	const AStudyGameState* StudyGameState{GetStudyGameState()};
	const FDateTime StudyNow{StudyGameState
								 ? StudyGameState->GetStudyNow()
								 : FDateTime::UtcNow()};
	const double DaysSinceSolve{
		(StudyNow - SolveTime).GetTotalSeconds() / SecondsPerDay};
	return FMath::Pow(0.5, DaysSinceSolve / RetentionHalfLifeDays);
}

void UStudyCastleComponent::SetCastleMaterialScalar(
	FName ParameterName, float Value)
{
	TArray<UStaticMeshComponent*> MeshComponents;
	GetOwner()->GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		for (int32 MaterialIndex{0};
			 MaterialIndex < MeshComponent->GetNumMaterials();
			 ++MaterialIndex)
		{
			UMaterialInstanceDynamic* Material{
				MeshComponent->CreateDynamicMaterialInstance(
					MaterialIndex)};
			if (Material)
			{
				Material->SetScalarParameterValue(
					ParameterName, Value);
			}
		}
	}
}

void UStudyCastleComponent::PlaySoundAtCastle(
	USoundBase* Sound) const
{
	if (!Sound)
	{
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(
		this, Sound, GetOwner()->GetActorLocation());
}

AStudyGameState* UStudyCastleComponent::GetStudyGameState() const
{
	return GetWorld()->GetGameState<AStudyGameState>();
}
