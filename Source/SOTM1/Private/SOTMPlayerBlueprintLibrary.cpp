#include "SOTMPlayerBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "SOTMPlayerVitalComponent.h"

USOTMPlayerStateSubsystem* USOTMPlayerBlueprintLibrary::GetPlayerStateSubsystem(
	const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
}

USOTMPlayerVitalComponent* USOTMPlayerBlueprintLibrary::GetVitalComponent(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USOTMPlayerVitalComponent>() : nullptr;
}

int32 USOTMPlayerBlueprintLibrary::GetAvailableCoins(const UObject* WorldContextObject)
{
	const USOTMPlayerStateSubsystem* State = GetPlayerStateSubsystem(WorldContextObject);
	return State ? State->GetAvailableCoins() : 0;
}

int32 USOTMPlayerBlueprintLibrary::GetLifetimeCoinsCollected(const UObject* WorldContextObject)
{
	const USOTMPlayerStateSubsystem* State = GetPlayerStateSubsystem(WorldContextObject);
	return State ? State->GetLifetimeCoinsCollected() : 0;
}

bool USOTMPlayerBlueprintLibrary::TryCollectCoin(
	const UObject* WorldContextObject,
	const FGuid PersistentCoinId,
	AActor* Collector,
	const int32 CoinValue)
{
	if (USOTMPlayerStateSubsystem* State = GetPlayerStateSubsystem(WorldContextObject))
	{
		return State->TryCollectCoin(PersistentCoinId, Collector, CoinValue);
	}
	return false;
}

bool USOTMPlayerBlueprintLibrary::IsCoinCollected(
	const UObject* WorldContextObject,
	const FGuid PersistentCoinId)
{
	const USOTMPlayerStateSubsystem* State = GetPlayerStateSubsystem(WorldContextObject);
	return State && State->IsCoinCollected(PersistentCoinId);
}

float USOTMPlayerBlueprintLibrary::ApplyPlayerDamage(
	const UObject* WorldContextObject,
	AActor* Target,
	const float Damage,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	return Target && Damage > 0.0f
		? UGameplayStatics::ApplyDamage(Target, Damage, InstigatedBy, DamageCauser, nullptr)
		: 0.0f;
}

void USOTMPlayerBlueprintLibrary::AcquirePlayerInputLock(
	const UObject* WorldContextObject,
	const ESOTMInputLockReason Reason)
{
	if (USOTMPlayerStateSubsystem* State = GetPlayerStateSubsystem(WorldContextObject))
	{
		State->AcquireInputLock(Reason);
	}
}

void USOTMPlayerBlueprintLibrary::ReleasePlayerInputLock(
	const UObject* WorldContextObject,
	const ESOTMInputLockReason Reason)
{
	if (USOTMPlayerStateSubsystem* State = GetPlayerStateSubsystem(WorldContextObject))
	{
		State->ReleaseInputLock(Reason);
	}
}

bool USOTMPlayerBlueprintLibrary::ActivateCheckpointAtTransform(
	const UObject* WorldContextObject,
	const FName CheckpointId,
	const FName MapPackageName,
	const FTransform& RespawnTransform)
{
	if (USOTMPlayerStateSubsystem* State = GetPlayerStateSubsystem(WorldContextObject))
	{
		return State->ActivateCheckpoint(CheckpointId, MapPackageName, RespawnTransform, true);
	}
	return false;
}
