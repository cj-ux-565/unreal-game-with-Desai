#pragma once

#include "CoreMinimal.h"
#include "SOTMCousinTypes.generated.h"

UENUM(BlueprintType)
enum class ESOTMCousinAIState : uint8
{
	Idle,
	Patrol,
	Chase,
	Catch,
	Disabled
};

