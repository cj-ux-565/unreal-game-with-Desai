#include "Coin/SOTMCoinPickup.h"

#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerStateSubsystem.h"

ASOTMCoinPickup::ASOTMCoinPickup()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASOTMCoinPickup::BeginPlay()
{
	Super::BeginPlay();

	if (!PersistentCoinId.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SOTM Coin: placed pickup %s has no PersistentCoinId and cannot be collected."), *GetPathName());
		DisableCollectedPickup();
		return;
	}

	BoundStateSubsystem = USOTMPlayerBlueprintLibrary::GetPlayerStateSubsystem(this);
	if (BoundStateSubsystem)
	{
		BoundStateSubsystem->OnCoinsChanged.AddUniqueDynamic(this, &ASOTMCoinPickup::HandleCoinsChanged);
	}
	RefreshPersistentAvailability();
}

void ASOTMCoinPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundStateSubsystem)
	{
		BoundStateSubsystem->OnCoinsChanged.RemoveDynamic(this, &ASOTMCoinPickup::HandleCoinsChanged);
	}
	BoundStateSubsystem = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ASOTMCoinPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bCollectionInProgress || !OtherActor)
	{
		return;
	}

	if (!USOTMPlayerBlueprintLibrary::TryCollectCoin(this, PersistentCoinId, OtherActor, CoinValue))
	{
		return;
	}

	// Shut collision down before feedback so repeated overlap callbacks cannot
	// award this runtime Coin instance twice.
	bCollectionInProgress = true;
	DisableCollectedPickup();
	if (PickupSound)
	{
		UGameplayStatics::PlaySound2D(this, PickupSound);
	}
	Destroy();
}

void ASOTMCoinPickup::HandleCoinsChanged(int32 AvailableCoins, int32 LifetimeCoinsCollected)
{
	(void)AvailableCoins;
	(void)LifetimeCoinsCollected;
	RefreshPersistentAvailability();
}

void ASOTMCoinPickup::RefreshPersistentAvailability()
{
	if (!bCollectionInProgress && BoundStateSubsystem && BoundStateSubsystem->IsCoinCollected(PersistentCoinId))
	{
		DisableCollectedPickup();
	}
}

void ASOTMCoinPickup::DisableCollectedPickup()
{
	bCollectionInProgress = true;
	SetActorEnableCollision(false);
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive)
		{
			Primitive->SetGenerateOverlapEvents(false);
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	SetActorHiddenInGame(true);
}
