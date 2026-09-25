#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTMCoinPickup.generated.h"

class USoundBase;
class USOTMPlayerStateSubsystem;

/**
 * Native runtime contract for the existing /Game/Blueprints/Coin asset.
 * Phase 2 adds a stable per-instance identity while preserving the Phase 1
 * pickup presentation and placement.
 */
UCLASS(Blueprintable)
class SOTM1_API ASOTMCoinPickup : public AActor
{
	GENERATED_BODY()

public:
	ASOTMCoinPickup();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintPure, Category="SOTM|Coin")
	FGuid GetPersistentCoinId() const { return PersistentCoinId; }

protected:
	/** Temporary default because the client has not approved denominations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Coin", meta=(ClampMin="1"))
	int32 CoinValue = 1;

	/** Existing project pickup sound, assigned on the Blueprint default object. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SOTM|Coin")
	TObjectPtr<USoundBase> PickupSound;

	/** Assigned once to placed production instances by the CH1 Editor migration. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="SOTM|Coin", SaveGame)
	FGuid PersistentCoinId;

	/** Visual-only spin so pickups read at a glance in the dark forest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Coin", meta=(ClampMin="0.0"))
	float SpinDegreesPerSecond = 120.0f;

private:
	UFUNCTION()
	void HandleCoinsChanged(int32 AvailableCoins, int32 LifetimeCoinsCollected);

	void RefreshPersistentAvailability();
	void DisableCollectedPickup();

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerStateSubsystem> BoundStateSubsystem;

	UPROPERTY(Transient)
	bool bCollectionInProgress = false;
};
