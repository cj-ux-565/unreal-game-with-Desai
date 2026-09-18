#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTMGameOverWidget.generated.h"

/**
 * Minimal production-safe Game Over choice surface. It intentionally contains
 * no final art and delegates retry/routing policy to the player-state subsystem.
 */
UCLASS()
class SOTM1_API USOTMGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

};
