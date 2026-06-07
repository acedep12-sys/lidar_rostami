// LeaderCharacter.cpp — Squad leader with NavInvoker, waypoints, player pacing
#include "Characters/LeaderCharacter.h"
#include "Components/HealthComponent.h"
#include "NavigationInvokerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SquadAITuning.h"
#include "Engine/World.h"

ALeaderCharacter::ALeaderCharacter()
{
	Faction = ESquadFaction::Ally;
	AccuracyMultiplier = 0.55f; // Better than regular allies

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f; // 4 Hz — cheap

	// NavigationInvoker — generates NavMesh around the leader
	NavInvokerComp = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
}

void ALeaderCharacter::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();

	// Leader is immortal
	if (HealthComp)
	{
		HealthComp->bInvincible = true;
	}

	// NavInvoker config
	if (NavInvokerComp)
	{
		NavInvokerComp->SetGenerationRadii(T->NavInvokerRadius, T->NavInvokerRemovalRadius);
	}

	// Pacing config
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

// =====================================================================
//  PACING — speed varies with player distance (TLOU / Half-Life feel)
// =====================================================================
void ALeaderCharacter::UpdatePacing()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC) return;

	const float PlayerDist = GetPlayerDistance();

	if (PlayerDist > WaitForPlayerRadius)
	{
		// Player too far — stop and wait
		CMC->MaxWalkSpeed = 0.f;
	}
	else if (PlayerDist > FullSpeedRadius)
	{
		// Blend speed based on distance band
		const float Alpha = FMath::Clamp(
			(PlayerDist - FullSpeedRadius) / (WaitForPlayerRadius - FullSpeedRadius),
			0.f, 1.f);
		CMC->MaxWalkSpeed = FMath::Lerp(NormalSpeed, WaitingSpeed, Alpha);
	}
	else
	{
		CMC->MaxWalkSpeed = NormalSpeed;
	}
}

// =====================================================================
//  WAYPOINT STATE MACHINE
// =====================================================================
void ALeaderCharacter::UpdateWaypointState(float DeltaTime)
{
	if (IsQuestComplete()) return;
	if (QuestWaypoints.Num() == 0) return;

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
		if (WP.bRequireAreaClear)
		{
			WaypointState = ELeaderWaypointState::WaitingForClear;
		}
		else
		{
			LingerTimer += DeltaTime;
			if (LingerTimer >= WP.LingerSeconds)
			{
				AdvanceToNextWaypoint();
			}
		}
		break;
	}

	case ELeaderWaypointState::WaitingForClear:
	{
		if (IsCurrentAreaClear())
		{
			const FQuestWaypoint& WP = QuestWaypoints[CurrentWaypointIndex];
			LingerTimer += DeltaTime;
			if (LingerTimer >= WP.LingerSeconds)
			{
				AdvanceToNextWaypoint();
			}
		}
		else
		{
			LingerTimer = 0.f;
		}
		break;
	}

	case ELeaderWaypointState::Completed:
		break;
	}
}

// =====================================================================
//  API
// =====================================================================
void ALeaderCharacter::SetQuestWaypoints(const TArray<FQuestWaypoint>& Waypoints)
{
	QuestWaypoints = Waypoints;
	CurrentWaypointIndex = 0;
	WaypointState = ELeaderWaypointState::NotReached;
}

void ALeaderCharacter::BeginQuest()
{
	if (QuestWaypoints.Num() > 0)
	{
		WaypointState = ELeaderWaypointState::Moving;
	}
}

bool ALeaderCharacter::AdvanceToNextWaypoint()
{
	CurrentWaypointIndex++;
	LingerTimer = 0.f;

	if (CurrentWaypointIndex >= QuestWaypoints.Num())
	{
		WaypointState = ELeaderWaypointState::Completed;
		return false;
	}

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

	// Use SoldierRegistry for O(K) check when available (Batch 5)
	// For now: check perceived enemies
	// The leader's AI controller handles this via the perception system
	// This is a simplified check — the controller's IsCurrentAreaClear is more robust
	return true; // Placeholder — connected by controller
}

FVector ALeaderCharacter::GetCurrentWaypointLocation() const
{
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) return GetActorLocation();
	return QuestWaypoints[CurrentWaypointIndex].Location;
}

float ALeaderCharacter::GetPlayerDistance() const
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return 0.f;
	return FVector::Dist(GetActorLocation(), Player->GetActorLocation());
}

bool ALeaderCharacter::IsQuestComplete() const
{
	return WaypointState == ELeaderWaypointState::Completed;
}
