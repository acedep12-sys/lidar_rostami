// AimComponent.cpp — Critically damped spring aim with body yaw separation
#include "Components/AimComponent.h"
#include "SquadAITuning.h"
#include "GameFramework/Character.h"
#include "AI/SoldierAIController.h"

UAimComponent::UAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAimComponent::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();
	BodyYawRate = T->AimBodyYawRate;
	SpringStiffness = T->AimSpringStiffness;
	SpringDampingRatio = T->AimSpringDampingRatio;
	MaxAimOffsetYaw = T->AimSpineConeDegrees;
	MaxAimErrorDegrees = T->AimMaxErrorDegrees;

	if (AActor* Owner = GetOwner())
	{
		CurrentBodyYaw = Owner->GetActorRotation().Yaw;
		SpringPos = Owner->GetActorForwardVector();
	}
}

void UAimComponent::SetAimTarget(FVector WorldTarget)
{
	AimTarget = WorldTarget;
	bHasTarget = true;
}

void UAimComponent::ClearAimTarget()
{
	bHasTarget = false;
}

void UAimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (bHasTarget)
	{
		DesiredAimDir = (AimTarget - Owner->GetActorLocation()).GetSafeNormal();
		if (DesiredAimDir.IsNearlyZero())
		{
			DesiredAimDir = Owner->GetActorForwardVector();
		}
	}
	else
	{
		DesiredAimDir = Owner->GetActorForwardVector();
	}

	// LOD check: skip expensive spring for Ambient/Dormant AI (suggestion 7.1)
	// They snap instantly instead — invisible to the player since they're far away
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		if (!CachedAIController.IsValid())
		{
			CachedAIController = Cast<ASoldierAIController>(Pawn->GetController());
		}

		if (CachedAIController.IsValid() && CachedAIController->CurrentLODBucket >= 3) // 3 = Low, 4 = VeryLow, 5 = Dormant
		{
			// Snap — no spring, no body yaw separation
			SpringPos = DesiredAimDir;
			SpringVel = FVector::ZeroVector;
			return;
		}
	}

	UpdateBodyYaw(DeltaTime);
	UpdateSpineSpring(DeltaTime);
}

// =====================================================================
//  BODY YAW — only rotates when the desired aim exceeds the spine cone
// =====================================================================
void UAimComponent::UpdateBodyYaw(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const float DesiredYaw = DesiredAimDir.Rotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentBodyYaw, DesiredYaw);

	// Only rotate body if the aim is outside the spine cone
	if (FMath::Abs(DeltaYaw) > MaxAimOffsetYaw)
	{
		// How far past the cone are we?
		const float Overshoot = FMath::Abs(DeltaYaw) - MaxAimOffsetYaw;
		const float MaxStep = BodyYawRate * DeltaTime;
		const float Step = FMath::Min(Overshoot, MaxStep) * FMath::Sign(DeltaYaw);

		CurrentBodyYaw += Step;
		CurrentBodyYaw = FMath::UnwindDegrees(CurrentBodyYaw);

		// Apply to character (only yaw, don't touch pitch/roll)
		FRotator OwnerRot = Owner->GetActorRotation();
		OwnerRot.Yaw = CurrentBodyYaw;

		if (ACharacter* Char = Cast<ACharacter>(Owner))
		{
			// Use SetActorRotation to properly update movement component
			Char->SetActorRotation(OwnerRot);
		}
		else
		{
			Owner->SetActorRotation(OwnerRot);
		}
	}
	else
	{
		// Body stays put — spine handles the aiming
		CurrentBodyYaw = Owner->GetActorRotation().Yaw;
	}
}

// =====================================================================
//  SPINE SPRING — critically damped, semi-implicit Euler
// =====================================================================
void UAimComponent::UpdateSpineSpring(float DeltaTime)
{
	if (DeltaTime <= 0.f) return;

	// Critically Damped Semi-Implicit Integration
	// This math is perfectly stable at any framerate (even 10fps or variable dt)
	const FVector SpringForce = (DesiredAimDir - SpringPos) * SpringStiffness;
	
	// Real critical damping calculation: damping = 2 * sqrt(stiffness)
	const float RealDamping = 2.f * FMath::Sqrt(SpringStiffness) * SpringDampingRatio;
	const FVector DampingForce = SpringVel * RealDamping;
	
	const FVector Acceleration = SpringForce - DampingForce;
	
	SpringVel += Acceleration * DeltaTime;
	SpringPos = (SpringPos + SpringVel * DeltaTime).GetSafeNormal();

	if (SpringPos.IsNearlyZero())
	{
		SpringPos = DesiredAimDir;
		SpringVel = FVector::ZeroVector;
	}
}

// =====================================================================
//  OUTPUTS
// =====================================================================
FRotator UAimComponent::GetAimedRotation() const
{
	FRotator Aimed = SpringPos.Rotation();

	// Add accuracy error
	if (MaxAimErrorDegrees > 0.f)
	{
		Aimed.Yaw += FMath::RandRange(-MaxAimErrorDegrees, MaxAimErrorDegrees);
		Aimed.Pitch += FMath::RandRange(-MaxAimErrorDegrees * 0.5f, MaxAimErrorDegrees * 0.5f);
	}

	return Aimed;
}

FRotator UAimComponent::GetSpineAimOffset() const
{
	// The offset is the difference between where the body faces and where the spine aims
	AActor* Owner = GetOwner();
	if (!Owner) return FRotator::ZeroRotator;

	const FRotator BodyRot = FRotator(0.f, CurrentBodyYaw, 0.f);
	const FRotator AimRot = SpringPos.Rotation();

	FRotator Offset;
	Offset.Yaw = FMath::FindDeltaAngleDegrees(BodyRot.Yaw, AimRot.Yaw);
	Offset.Pitch = AimRot.Pitch; // Full pitch in the offset

	// Clamp to spine cone
	Offset.Yaw = FMath::Clamp(Offset.Yaw, -MaxAimOffsetYaw, MaxAimOffsetYaw);
	Offset.Pitch = FMath::Clamp(Offset.Pitch, -MaxAimOffsetPitch, MaxAimOffsetPitch);

	return Offset;
}

bool UAimComponent::IsAimedAtTarget(float ToleranceDegrees) const
{
	if (!bHasTarget) return false;

	const float AngleDiff = FMath::RadiansToDegrees(
		FMath::Acos(FVector::DotProduct(SpringPos, DesiredAimDir)));
	return AngleDiff <= ToleranceDegrees;
}
