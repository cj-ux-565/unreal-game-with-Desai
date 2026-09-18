#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerBlueprintLibrary.generated.h"

class USOTMPlayerVitalComponent;

/**
 * Stable Blueprint API for future AI, HUD, objectives, cinematics and jump
 * scares. Callers do not need hard references to the production character.
 */
UCLASS()
class SOTM1_API USOTMPlayerBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="SOTM|Player", meta=(WorldContext="WorldContextObject"))
	static USOTMPlayerStateSubsystem* GetPlayerStateSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="SOTM|Player")
	static USOTMPlayerVitalComponent* GetVitalComponent(AActor* Actor);

	UFUNCTION(BlueprintPure, Category="SOTM|Coin", meta=(WorldContext="WorldContextObject"))
	static int32 GetAvailableCoins(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="SOTM|Coin", meta=(WorldContext="WorldContextObject"))
	static int32 GetLifetimeCoinsCollected(const UObject* WorldContextObject);

	/** Validates the collector against the Player System's bound production pawn. */
	UFUNCTION(BlueprintCallable, Category="SOTM|Coin", meta=(WorldContext="WorldContextObject"))
	static bool TryCollectCoin(
		const UObject* WorldContextObject,
		FGuid PersistentCoinId,
		AActor* Collector,
		int32 CoinValue = 1);

	UFUNCTION(BlueprintPure, Category="SOTM|Coin", meta=(WorldContext="WorldContextObject"))
	static bool IsCoinCollected(const UObject* WorldContextObject, FGuid PersistentCoinId);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Damage", meta=(WorldContext="WorldContextObject"))
	static float ApplyPlayerDamage(
		const UObject* WorldContextObject,
		AActor* Target,
		float Damage,
		AController* InstigatedBy,
		AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Input", meta=(WorldContext="WorldContextObject"))
	static void AcquirePlayerInputLock(const UObject* WorldContextObject, ESOTMInputLockReason Reason);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Input", meta=(WorldContext="WorldContextObject"))
	static void ReleasePlayerInputLock(const UObject* WorldContextObject, ESOTMInputLockReason Reason);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Checkpoint", meta=(WorldContext="WorldContextObject"))
	static bool ActivateCheckpointAtTransform(
		const UObject* WorldContextObject,
		FName CheckpointId,
		FName MapPackageName,
		const FTransform& RespawnTransform);
};
