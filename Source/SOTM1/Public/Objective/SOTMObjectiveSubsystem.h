#pragma once

#include "CoreMinimal.h"
#include "Demo/SOTMPhase4Types.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTMObjectiveSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESOTMObjectiveState : uint8
{
	Locked,
	Active,
	Completed
};

USTRUCT(BlueprintType)
struct FSOTMObjectiveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="SOTM|Objective")
	FName ObjectiveId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="SOTM|Objective")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="SOTM|Objective")
	int32 CurrentProgress = 0;

	UPROPERTY(BlueprintReadOnly, Category="SOTM|Objective")
	int32 RequiredProgress = 0;

	UPROPERTY(BlueprintReadOnly, Category="SOTM|Objective")
	ESOTMObjectiveState State = ESOTMObjectiveState::Locked;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSOTMObjectiveChangedSignature,
	FSOTMObjectiveData,
	Objective);

/**
 * Authoritative, event-driven Chapter objective state. The Phase 2 Coin
 * objective is reconstructed from persistent lifetime/unique Coin state, so a
 * future currency spend can never undo completion.
 */
UCLASS(BlueprintType)
class SOTM1_API USOTMObjectiveSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const FName CollectAllForestCoinsId;
	static const FName UnlockSpeedBoostId;
	static const FName FindChestId;
	static const FName ObtainGateKeyId;
	static const FName ReachGateId;
	static const FName DemoCompleteId;
	static constexpr int32 TotalForestCoins = 330;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="SOTM|Objective")
	void SetForestObjectiveActive(bool bActive);

	UFUNCTION(BlueprintPure, Category="SOTM|Objective")
	FSOTMObjectiveData GetCollectAllForestCoinsObjective() const { return CollectAllForestCoins; }

	UFUNCTION(BlueprintPure, Category="SOTM|Objective")
	TArray<FSOTMObjectiveData> GetChapterOneObjectives() const;

	UFUNCTION(BlueprintPure, Category="SOTM|Objective")
	FSOTMObjectiveData GetActiveChapterOneObjective() const;

	UFUNCTION(BlueprintCallable, Category="SOTM|Objective|Phase 4")
	ESOTMPhase4ActionResult TryOpenPhase4Chest();

	UFUNCTION(BlueprintCallable, Category="SOTM|Objective|Phase 4")
	ESOTMPhase4ActionResult TryUnlockPhase4Gate();

	UFUNCTION(BlueprintCallable, Category="SOTM|Objective|Phase 4")
	ESOTMPhase4ActionResult TryCompletePhase4Demo();

	UFUNCTION(BlueprintPure, Category="SOTM|Objective|Phase 4")
	FText GetGateRequirementFeedback() const;

	UFUNCTION(BlueprintPure, Category="SOTM|Objective")
	bool IsForestObjectiveActive() const { return bForestObjectiveActive; }

	UPROPERTY(BlueprintAssignable, Category="SOTM|Objective|Events")
	FSOTMObjectiveChangedSignature OnObjectiveChanged;

private:
	UFUNCTION()
	void HandleCoinsChanged(int32 AvailableCoins, int32 LifetimeCoinsCollected);

	UFUNCTION()
	void HandleSpeedBoostChanged(bool bUnlocked, int32 Level);

	UFUNCTION()
	void HandlePhase4ProgressChanged(
		bool bChestOpened,
		bool bHasGateKey,
		bool bGateUnlocked,
		bool bDemoCompleted);

	void RefreshFromPersistentCoinState(bool bForceBroadcast);
	void RefreshPhase4Objectives(bool bForceBroadcast);
	void BroadcastIfChanged(
		const FSOTMObjectiveData& Previous,
		const FSOTMObjectiveData& Current,
		bool bForceBroadcast);

	UPROPERTY(Transient)
	TObjectPtr<class USOTMPlayerStateSubsystem> PlayerState;

	UPROPERTY(Transient)
	FSOTMObjectiveData CollectAllForestCoins;

	UPROPERTY(Transient)
	FSOTMObjectiveData UnlockSpeedBoost;

	UPROPERTY(Transient)
	FSOTMObjectiveData FindChest;

	UPROPERTY(Transient)
	FSOTMObjectiveData ObtainGateKey;

	UPROPERTY(Transient)
	FSOTMObjectiveData ReachGate;

	UPROPERTY(Transient)
	FSOTMObjectiveData DemoComplete;

	bool bForestObjectiveActive = false;
};
