// AutoSpawnZone.cpp — One-drop encounter: spawns zone + spawner + coordinator + spawn points
#include "Quest/AutoSpawnZone.h"
#include "Quest/QuestZone.h"
#include "Quest/WaveSpawner.h"
#include "Quest/WaveTemplate.h"
#include "AI/EnemyAttackCoordinator.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"

AAutoSpawnZone::AAutoSpawnZone()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	EditorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	EditorIcon->SetupAttachment(Root);
	EditorIcon->bIsEditorOnly = true;
#endif
}

void AAutoSpawnZone::BeginPlay()
{
	Super::BeginPlay();
	SpawnChildActors();
}

void AAutoSpawnZone::SpawnChildActors()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Center = GetActorLocation();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 1. Spawn QuestZone
	SpawnedZone = World->SpawnActor<AQuestZone>(AQuestZone::StaticClass(), Center, FRotator::ZeroRotator, Params);
	if (SpawnedZone)
	{
		SpawnedZone->ZoneTag = Tag;
		SpawnedZone->Shape = EZoneShape::Box;
		SpawnedZone->BoxExtent = ZoneExtents;
	}

	// 2. Generate spawn point locations
	TArray<FVector> SpawnLocs = GenerateSpawnPointLocations();

	// 3. Spawn WaveSpawner
	SpawnedSpawner = World->SpawnActor<AWaveSpawner>(AWaveSpawner::StaticClass(), Center, FRotator::ZeroRotator, Params);
	if (SpawnedSpawner)
	{
		FQuestTag SpawnerTag;
		SpawnerTag.Tag = FName(*(Tag.Tag.ToString() + TEXT("_Spawner")));
		SpawnedSpawner->SpawnerTag = SpawnerTag;
		SpawnedSpawner->WaveTemplate = WaveTemplate;
		SpawnedSpawner->DefaultEnemyClass = DefaultEnemyClass;
		SpawnedSpawner->SpawnPointLocations = SpawnLocs;
	}

	// 4. Spawn EnemyAttackCoordinator (optional)
	if (bUseAttackCoordinator)
	{
		SpawnedCoordinator = World->SpawnActor<AEnemyAttackCoordinator>(
			AEnemyAttackCoordinator::StaticClass(), Center, FRotator::ZeroRotator, Params);

		if (SpawnedCoordinator)
		{
			SpawnedCoordinator->AttackPointLocation = Center;
			SpawnedCoordinator->StagingAreaLocation = Center + FVector(SpawnPointRadius * 0.7f, 0, 0);

			// Link spawner to coordinator
			if (SpawnedSpawner)
			{
				SpawnedSpawner->Coordinator = SpawnedCoordinator;
			}
		}
	}

	UE_LOG(LogSquadAI, Log, TEXT("AutoSpawnZone '%s': spawned zone + spawner + %d spawn points + %s coordinator"),
		*Tag.Tag.ToString(), SpawnLocs.Num(), bUseAttackCoordinator ? TEXT("with") : TEXT("no"));
}

TArray<FVector> AAutoSpawnZone::GenerateSpawnPointLocations() const
{
	TArray<FVector> Result;
	const FVector Center = GetActorLocation();

	for (int32 i = 0; i < NumSpawnPoints; ++i)
	{
		const float Angle = (360.f / NumSpawnPoints) * i;
		const float Rad = FMath::DegreesToRadians(Angle);
		FVector Offset(FMath::Cos(Rad) * SpawnPointRadius, FMath::Sin(Rad) * SpawnPointRadius, 0.f);
		Result.Add(Center + Offset);
	}

	return Result;
}

#if WITH_EDITORONLY_DATA
void AAutoSpawnZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Clear old editor arrows
	for (UArrowComponent* Arrow : EditorArrows)
	{
		if (Arrow) Arrow->DestroyComponent();
	}
	EditorArrows.Empty();

	// Generate preview arrows showing spawn point locations
	TArray<FVector> SpawnLocs = GenerateSpawnPointLocations();
	for (int32 i = 0; i < SpawnLocs.Num(); ++i)
	{
		UArrowComponent* Arrow = NewObject<UArrowComponent>(this);
		Arrow->SetupAttachment(RootComponent);
		Arrow->RegisterComponent();
		Arrow->bIsEditorOnly = true;
		Arrow->ArrowColor = FColor::Red;
		Arrow->ArrowSize = 1.5f;
		Arrow->SetWorldLocation(SpawnLocs[i]);

		// Point toward center
		FVector ToCenter = (GetActorLocation() - SpawnLocs[i]).GetSafeNormal();
		Arrow->SetWorldRotation(ToCenter.Rotation());

		EditorArrows.Add(Arrow);
	}
}
#endif
