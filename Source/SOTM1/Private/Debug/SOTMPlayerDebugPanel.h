#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class IInputProcessor;
class SWidget;
class USOTMPlayerFoundationWorldSubsystem;

namespace SOTMPlayerDebugPanel
{
	TSharedRef<SWidget> CreatePanel(USOTMPlayerFoundationWorldSubsystem* OwnerSubsystem);
	TSharedRef<IInputProcessor> CreateInputPreprocessor(USOTMPlayerFoundationWorldSubsystem* OwnerSubsystem);
}

#endif
