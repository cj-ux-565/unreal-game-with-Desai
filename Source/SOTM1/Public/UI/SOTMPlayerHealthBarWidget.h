#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTMPlayerHealthBarWidget.generated.h"

class UProgressBar;
class USOTMPlayerStateSubsystem;
class USOTMPlayerVitalComponent;
enum class ESOTMInputLockReason : uint8;

/**
 * Event-driven native parent for the existing production blue health bar.
 * The Widget Blueprint remains responsible for all presentation and layout.
 */
UCLASS(Abstract, Blueprintable)
class SOTM1_API USOTMPlayerHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BindToProductionPlayer();
	void RefreshHealth(float CurrentHealth, float MaximumHealth);

	FTimerHandle PlayerBindingRetryTimer;
	int32 PlayerBindingRetryCount = 0;

	UFUNCTION()
	void HandleHealthChanged(
		USOTMPlayerVitalComponent* VitalComponent,
		float PreviousHealth,
		float CurrentHealth,
		float MaximumHealth);

	UFUNCTION()
	void HandleInputLocksChanged(bool bInputLocked, TArray<ESOTMInputLockReason> ActiveReasons);

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerVitalComponent> BoundVitalComponent;

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerStateSubsystem> BoundPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthProgressBar;

	ESlateVisibility NormalVisibility = ESlateVisibility::Visible;
};
