#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTMDemoCompleteWidget.generated.h"

class UTextBlock;

/** Final demo presentation only. No store URL is invented. */
UCLASS(NotBlueprintable)
class SOTM1_API USOTMDemoCompleteWidget final : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleBuyFullGame();

	UFUNCTION()
	void HandleReturnToMainMenu();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StoreStatusText;
};
