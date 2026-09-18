#pragma once

#include "CoreMinimal.h"
#include "Ability/SOTMSpeedBoostTypes.h"
#include "Blueprint/UserWidget.h"
#include "Objective/SOTMObjectiveSubsystem.h"
#include "SOTMUpgradeStationWidget.generated.h"

class SButton;
class STextBlock;
class USOTMObjectiveSubsystem;
class USOTMPlayerStateSubsystem;

/** Functional, data-driven Phase 3 station screen built in the approved gothic layout. */
UCLASS()
class SOTM1_API USOTMUpgradeStationWidget final : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION()
	void HandleCoinsChanged(int32 AvailableCoins, int32 LifetimeCoinsCollected);

	UFUNCTION()
	void HandleOwnershipChanged(bool bUnlocked, int32 Level);

	UFUNCTION()
	void HandleObjectiveChanged(FSOTMObjectiveData Objective);

	FReply HandleUnlockClicked();
	FReply HandleCloseClicked();
	void RefreshPresentation();
	bool CanPurchase() const;
	FText GetRequirementText() const;

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerStateSubsystem> PlayerState;

	UPROPERTY(Transient)
	TObjectPtr<USOTMObjectiveSubsystem> ObjectiveState;

	TSharedPtr<STextBlock> OwnershipText;
	TSharedPtr<STextBlock> LevelText;
	TSharedPtr<STextBlock> AvailableCoinsText;
	TSharedPtr<STextBlock> CostText;
	TSharedPtr<STextBlock> RequirementText;
	TSharedPtr<SButton> UnlockButton;
	TSharedPtr<STextBlock> UnlockButtonText;
	ESOTMSpeedBoostPurchaseResult LastPurchaseResult = ESOTMSpeedBoostPurchaseResult::ObjectiveIncomplete;
	bool bHasAttemptedPurchase = false;
};
