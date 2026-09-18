#include "SOTMCheckpoint.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Misc/PackageName.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"

ASOTMCheckpoint::ASOTMCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("CheckpointTrigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(120.0f, 120.0f, 160.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(Trigger);
	RespawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
}

void ASOTMCheckpoint::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASOTMCheckpoint::HandleTriggerBeginOverlap);

	if (CheckpointId.IsNone())
	{
		CheckpointId = FName(*GetName());
	}
}

bool ASOTMCheckpoint::ActivateCheckpoint(AActor* PlayerActor)
{
	if (!IsValid(PlayerActor) || (bActivateOnlyOnce && bActivated))
	{
		return false;
	}

	APawn* Pawn = Cast<APawn>(PlayerActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	USOTMPlayerStateSubsystem* State =
		GameInstance ? GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	if (!State)
	{
		return false;
	}

	const bool bChanged = State->ActivateCheckpoint(
		CheckpointId,
		GetNormalizedMapPackageName(),
		RespawnPoint->GetComponentTransform(),
		true);
	bActivated = bActivated || bChanged;

	if (bChanged)
	{
		OnCheckpointActivated.Broadcast(CheckpointId, PlayerActor);
	}

	return bChanged;
}

void ASOTMCheckpoint::ResetCheckpoint()
{
	bActivated = false;
}

void ASOTMCheckpoint::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ActivateCheckpoint(OtherActor);
}

FName ASOTMCheckpoint::GetNormalizedMapPackageName() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return NAME_None;
	}

	const FString PackageName = World->GetOutermost()->GetName();
	const FString LongPath = FPackageName::GetLongPackagePath(PackageName);
	FString ShortName = FPackageName::GetShortName(PackageName);
	if (ShortName.StartsWith(TEXT("UEDPIE_")))
	{
		const int32 PrefixEnd = ShortName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 7);
		if (PrefixEnd != INDEX_NONE)
		{
			ShortName = ShortName.Mid(PrefixEnd + 1);
		}
	}
	return FName(*(LongPath / ShortName));
}
