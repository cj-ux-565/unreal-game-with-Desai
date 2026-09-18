#pragma once

#include "CoreMinimal.h"
#include "SOTMIsabelAITypes.generated.h"

UENUM(BlueprintType)
enum class ESOTMIsabelAIState : uint8
{
	Idle,
	Patrol,
	Investigate,
	Chase,
	Attack,
	JumpScare,
	SearchLastKnown,
	ReturnToPatrol
};

UENUM(BlueprintType)
enum class ESOTMPatrolTraversalMode : uint8
{
	Loop,
	BackAndForth
};
