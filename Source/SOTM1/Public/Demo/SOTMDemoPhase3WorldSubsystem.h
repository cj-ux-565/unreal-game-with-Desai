#pragma once

#include "CoreMinimal.h"
#include "Ability/SOTMSpeedBoostTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "SOTMDemoPhase3WorldSubsystem.generated.h"

class ASOTMTimmyUpgradeStation;
class UCharacterMovementComponent;
class UAudioComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class UParticleSystemComponent;
class USOTMPlayerStateSubsystem;
class USOTMUpgradeStationWidget;

/** CH1-only Phase 3 station, purchase, input and runtime Speed Boost coordinator. */
UCLASS()
class SOTM1_API USOTMDemoPhase3WorldSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category="SOTM|Phase 3")
	FSOTMStationPromptChangedSignature OnStationPromptChanged;

	UPROPERTY(BlueprintAssignable, Category="SOTM|Phase 3")
	FSOTMSpeedBoostStateChangedSignature OnSpeedBoostStateChanged;

	UFUNCTION(BlueprintPure, Category="SOTM|Phase 3")
	ESOTMSpeedBoostRuntimeState GetSpeedBoostState() const { return RuntimeState; }

	UFUNCTION(BlueprintPure, Category="SOTM|Phase 3")
	bool IsPlayerInStationRange() const { return bPlayerInStationRange; }

	UFUNCTION(BlueprintPure, Category="SOTM|Phase 3")
	bool IsUpgradeUIOpen() const { return UpgradeWidget != nullptr; }

	UFUNCTION(BlueprintCallable, Category="SOTM|Phase 3")
	ESOTMSpeedBoostPurchaseResult TryPurchaseSpeedBoost();

	UFUNCTION(BlueprintCallable, Category="SOTM|Phase 3")
	bool TryActivateSpeedBoost();

	UFUNCTION(BlueprintCallable, Category="SOTM|Phase 3")
	void OpenUpgradeUI();

	UFUNCTION(BlueprintCallable, Category="SOTM|Phase 3")
	void CloseUpgradeUI();

private:
	void InitializePhase3();
	void SpawnStationAtProductionTimmy();
	void BindProductionInput();
	void UnbindProductionInput();
	void HandleInteractInput();
	void HandleSpeedBoostInput();
	void HandleStationEntered(AActor* Actor);
	void HandleStationExited(AActor* Actor);
	void SetRuntimeState(ESOTMSpeedBoostRuntimeState NewState, float RemainingSeconds = 0.0f);
	void UpdateRuntimePresentation();
	void FinishActiveSpeedBoost();
	void FinishCooldown();
	void RestoreMovementSpeed();
	void ActivateBoostOwnedParticles();
	void DeactivateBoostOwnedParticles();
	void ResetRuntimeAfterDeath();
	UCharacterMovementComponent* ResolveMovementComponent() const;

#if !UE_BUILD_SHIPPING
	void BeginDevelopmentAcceptanceRoute();
	void BeginDevelopmentPersistenceCheck();
	void PositionForDevelopmentCousinChase();
	void LogDevelopmentAcceptanceState(const TCHAR* Label) const;
	void CaptureDevelopmentEvidence(const FString& Label) const;
#endif

	UFUNCTION()
	void HandleOwnershipChanged(bool bUnlocked, int32 Level);

	UFUNCTION()
	void HandlePlayerDeathStarted(AActor* PlayerActor);

	UFUNCTION()
	void HandlePlayerRespawned(AActor* PlayerActor);

	UFUNCTION()
	void HandleGameOver();

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerStateSubsystem> PlayerState;

	UPROPERTY(Transient)
	TObjectPtr<ASOTMTimmyUpgradeStation> StationActor;

	UPROPERTY(Transient)
	TObjectPtr<USOTMUpgradeStationWidget> UpgradeWidget;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractInputAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SpeedBoostInputAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Phase3InputContext;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveBoostAudio;

	TWeakObjectPtr<UEnhancedInputComponent> BoundEnhancedInput;
	uint32 InteractBindingHandle = 0;
	uint32 SpeedBoostBindingHandle = 0;
	bool bPlayerInStationRange = false;
	bool bUpgradeInputLockHeld = false;
	bool bPreviousMouseCursor = false;
	ESOTMSpeedBoostRuntimeState RuntimeState = ESOTMSpeedBoostRuntimeState::Locked;
	TWeakObjectPtr<UCharacterMovementComponent> BoostedMovement;
	float BaseSpeedBeforeBoost = 0.0f;
	float LastAppliedBoostedSpeed = 0.0f;
	// Cascade components this Speed Boost transitioned INACTIVE->ACTIVE.
	// Components already active (e.g. native Shift sprint) are never recorded.
	TArray<TWeakObjectPtr<UParticleSystemComponent>> BoostOwnedParticles;
	FTimerHandle InitializeTimer;
	FTimerHandle ActiveTimer;
	FTimerHandle CooldownTimer;
	FTimerHandle PresentationTimer;
};
