#pragma once

#include "CoreMinimal.h"
#include "SOTMSpeedBoostTypes.generated.h"

UENUM(BlueprintType)
enum class ESOTMSpeedBoostRuntimeState : uint8
{
	Locked,
	Ready,
	Active,
	Cooldown
};

UENUM(BlueprintType)
enum class ESOTMSpeedBoostPurchaseResult : uint8
{
	Success,
	AlreadyOwned,
	ObjectiveIncomplete,
	NotEnoughCoins,
	InvalidCost,
	NoActiveSave,
	SaveFailed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSOTMSpeedBoostOwnershipChangedSignature,
	bool, bUnlocked,
	int32, Level);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FSOTMSpeedBoostStateChangedSignature,
	ESOTMSpeedBoostRuntimeState, State,
	float, RemainingSeconds,
	float, NormalizedRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSOTMStationPromptChangedSignature,
	bool, bVisible);
