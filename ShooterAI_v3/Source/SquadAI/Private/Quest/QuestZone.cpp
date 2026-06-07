// QuestZone.cpp — Self-registering trigger volume
#include "Quest/QuestZone.h"
#include "Quest/QuestRegistry.h"
#include "Characters/SoldierCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AQuestZone::AQuestZone()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	BoxComp->SetGenerateOverlapEvents(true);
	BoxComp->SetHiddenInGame(true);
	BoxComp->ShapeColor = FColor::Cyan;
	RootComponent = BoxComp;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SphereComp->SetupAttachment(RootComponent);
	SphereComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereComp->SetGenerateOverlapEvents(true);
	SphereComp->SetHiddenInGame(true);
}

void AQuestZone::BeginPlay()
{
	Super::BeginPlay();
	ConfigureShape();

	UPrimitiveComponent* ActiveComp = (Shape == EZoneShape::Box)
		? static_cast<UPrimitiveComponent*>(BoxComp)
		: static_cast<UPrimitiveComponent*>(SphereComp);

	ActiveComp->OnComponentBeginOverlap.AddDynamic(this, &AQuestZone::OnOverlapBegin);
	ActiveComp->OnComponentEndOverlap.AddDynamic(this, &AQuestZone::OnOverlapEnd);

	// Self-register with QuestRegistry
	if (ZoneTag.IsValid())
	{
		if (UQuestRegistry* Reg = GetWorld()->GetSubsystem<UQuestRegistry>())
		{
			Reg->Register(ZoneTag, this);
		}
	}
}

void AQuestZone::EndPlay(const EEndPlayReason::Type Reason)
{
	if (ZoneTag.IsValid())
	{
		if (UQuestRegistry* Reg = GetWorld()->GetSubsystem<UQuestRegistry>())
		{
			Reg->Unregister(ZoneTag);
		}
	}
	Super::EndPlay(Reason);
}

void AQuestZone::ConfigureShape()
{
	if (Shape == EZoneShape::Box)
	{
		BoxComp->SetBoxExtent(BoxExtent);
		BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereComp->SetGenerateOverlapEvents(false);
	}
	else
	{
		SphereComp->SetSphereRadius(SphereRadius);
		SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoxComp->SetGenerateOverlapEvents(false);
	}
}

void AQuestZone::OnOverlapBegin(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherIdx, bool bSweep, const FHitResult& Hit)
{
	if (Other && Other != this) ActorsInZone.AddUnique(Other);
}

void AQuestZone::OnOverlapEnd(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherIdx)
{
	if (Other) ActorsInZone.Remove(Other);
}

bool AQuestZone::IsPlayerInZone() const
{
	APawn* P = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!P) return false;
	return ActorsInZone.ContainsByPredicate([P](const TWeakObjectPtr<AActor>& A) { return A.Get() == P; });
}

int32 AQuestZone::CountHostilesInZone() const
{
	int32 Count = 0;
	for (const auto& A : ActorsInZone)
	{
		if (const ASoldierCharacter* S = Cast<ASoldierCharacter>(A.Get()))
		{
			if (S->IsEnemy() && S->IsAlive()) Count++;
		}
	}
	return Count;
}

TArray<AActor*> AQuestZone::GetHostilesInZone() const
{
	TArray<AActor*> Result;
	for (const auto& A : ActorsInZone)
	{
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(A.Get()))
		{
			if (S->IsEnemy() && S->IsAlive()) Result.Add(S);
		}
	}
	return Result;
}

float AQuestZone::GetZoneRadius() const
{
	return (Shape == EZoneShape::Sphere) ? SphereRadius : BoxExtent.GetMax();
}
