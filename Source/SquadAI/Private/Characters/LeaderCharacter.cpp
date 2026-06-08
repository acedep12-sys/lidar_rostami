// LeaderCharacter.cpp — Squad leader with waypoints, player pacing
#include "Characters/LeaderCharacter.h"
#include "Components/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SquadAITuning.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Quest/QuestDirector.h"
#include "Quest/QuestMission.h"
#include "Quest/QuestObjective.h"
#include "Performance/SoldierRegistrySubsystem.h"

ALeaderCharacter::ALeaderCharacter()
{
	Faction = ESquadFaction::Ally;
	AccuracyMultiplier = 0.55f; 

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f; 
}

void ALeaderCharacter::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();

	if (HealthComp)
	{
		HealthComp->bInvincible = true;
	}

	WaitForPlayerRadius = T->LeaderWaitForPlayerRadius;
	FullSpeedRadius = T->LeaderFullSpeedRadius;
	NormalSpeed = T->LeaderNormalSpeed;
	WaitingSpeed = T->LeaderWaitingSpeed;
}

void ALeaderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdatePacing();
	UpdateWaypointState(DeltaTime);
}

void ALeaderCharacter::UpdatePacing()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) 
	{
		CMC->MaxWalkSpeed = NormalSpeed;
		return;
	}

	const float PlayerDist = GetPlayerDistance();
	if (PlayerDist > WaitForPlayerRadius) CMC->MaxWalkSpeed = 0.f;
	else if (PlayerDist > FullSpeedRadius)
	{
		const float Alpha = FMath::Clamp((PlayerDist - FullSpeedRadius) / (WaitForPlayerRadius - FullSpeedRadius), 0.f, 1.f);
		CMC->MaxWalkSpeed = FMath::Lerp(NormalSpeed, WaitingSpeed, Alpha);
	}
	else CMC->MaxWalkSpeed = NormalSpeed;
}

void ALeaderCharacter::UpdateWaypointState(float DeltaTime)
{
	if (IsQuestComplete() || QuestWaypoints.Num() == 0) return;

	switch (WaypointState)
	{
	case ELeaderWaypointState::NotReached:
	case ELeaderWaypointState::Moving:
		if (HasReachedCurrentWaypoint())
		{
			WaypointState = ELeaderWaypointState::Arrived;
			LingerTimer = 0.f;
		}
		break;
	case ELeaderWaypointState::Arrived:
	{
		const FQuestWaypoint& WP = QuestWaypoints[CurrentWaypointIndex];
		if (WP.bRequireAreaClear) WaypointState = ELeaderWaypointState::WaitingForClear;
		else { LingerTimer += DeltaTime; if (LingerTimer >= WP.LingerSeconds) AdvanceToNextWaypoint(); }
		break;
	}
	case ELeaderWaypointState::WaitingForClear:
		if (IsCurrentAreaClear()) { const FQuestWaypoint& WP = QuestWaypoints[CurrentWaypointIndex]; LingerTimer += DeltaTime; if (LingerTimer >= WP.LingerSeconds) AdvanceToNextWaypoint(); }
		else LingerTimer = 0.f;
		break;
	}
}

void ALeaderCharacter::SetQuestWaypoints(const TArray<FQuestWaypoint>& Waypoints) { QuestWaypoints = Waypoints; CurrentWaypointIndex = 0; WaypointState = ELeaderWaypointState::NotReached; }
void ALeaderCharacter::BeginQuest() { if (QuestWaypoints.Num() > 0) { WaypointState = ELeaderWaypointState::Moving; } }

bool ALeaderCharacter::AdvanceToNextWaypoint()
{
	CurrentWaypointIndex++;
	LingerTimer = 0.f;
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) { WaypointState = ELeaderWaypointState::Completed; return false; }
	WaypointState = ELeaderWaypointState::Moving;
	return true;
}

bool ALeaderCharacter::HasReachedCurrentWaypoint() const
{
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) return false;
	const FVector WPLoc = QuestWaypoints[CurrentWaypointIndex].Location;
	return FVector::Dist(GetActorLocation(), WPLoc) <= QuestWaypoints[CurrentWaypointIndex].AcceptanceRadius;
}

bool ALeaderCharacter::IsCurrentAreaClear() const
{
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) return true;
	const FQuestWaypoint& WP = QuestWaypoints[CurrentWaypointIndex];
	USoldierRegistrySubsystem* Reg = GetWorld()->GetSubsystem<USoldierRegistrySubsystem>();
	if (!Reg) return true;
	return Reg->CountHostilesNear(WP.Location, WP.AreaClearRadius, GetGenericTeamId().GetId()) == 0;
}

FVector ALeaderCharacter::GetCurrentWaypointLocation() const
{
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) return GetActorLocation();
	return QuestWaypoints[CurrentWaypointIndex].Location;
}

float ALeaderCharacter::GetPlayerDistance() const
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	return Player ? FVector::Dist(GetActorLocation(), Player->GetActorLocation()) : 0.f;
}

bool ALeaderCharacter::IsQuestComplete() const { return WaypointState == ELeaderWaypointState::Completed; }
