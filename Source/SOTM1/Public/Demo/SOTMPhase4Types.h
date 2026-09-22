#pragma once

#include "CoreMinimal.h"
#include "SOTMPhase4Types.generated.h"

UENUM(BlueprintType)
enum class ESOTMPhase4InteractableKind : uint8
{
	Chest,
	Gate
};

UENUM(BlueprintType)
enum class ESOTMPhase4ActionResult : uint8
{
	Success,
	AlreadyCompleted,
	PreviousObjectivesIncomplete,
	MissingSpeedBoost,
	MissingGateKey,
	SaveFailed,
	InvalidState
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSOTMPhase4PromptChangedSignature,
	bool, bVisible,
	FText, PromptText);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSOTMPhase4NotificationSignature,
	FText, Title,
	FText, Detail);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FSOTMPhase4GateOpenedSignature);
