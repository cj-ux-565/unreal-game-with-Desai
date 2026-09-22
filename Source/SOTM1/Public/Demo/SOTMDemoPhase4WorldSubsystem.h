#pragma once

#include "CoreMinimal.h"
#include "Demo/SOTMPhase4Types.h"
#include "Subsystems/WorldSubsystem.h"
#include "SOTMDemoPhase4WorldSubsystem.generated.h"

class AStaticMeshActor;
class ASOTMPhase4Interactable;
class UEnhancedInputComponent;
class UInputAction;
class USOTMDemoCompleteWidget;
class USOTMObjectiveSubsystem;
class USOTMPlayerStateSubsystem;

/** CH1-only Phase 4 chest, key, gate and safe demo-ending coordinator. */
UCLASS()
class SOTM1_API USOTMDemoPhase4WorldSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category="SOTM|Demo|Phase 4")
	FSOTMPhase4PromptChangedSignature OnPromptChanged;

	UPROPERTY(BlueprintAssignable, Category="SOTM|Demo|Phase 4")
	FSOTMPhase4NotificationSignature OnNotification;

	// Called when gate is opened - triggers Isabel encounter
	UPROPERTY(BlueprintAssignable, Category="SOTM|Demo|Phase 4")
	FSOTMPhase4GateOpenedSignature OnGateOpenedForIsabelEncounter;

	UFUNCTION(BlueprintPure, Category="SOTM|Demo|Phase 4")
	bool IsPlayerNearChest() const { return bNearChest; }

	UFUNCTION(BlueprintPure, Category="SOTM|Demo|Phase 4")
	bool IsPlayerNearGate() const { return bNearGate; }

#if !UE_BUILD_SHIPPING
	void BeginDevelopmentAcceptanceRoute();
	void BeginDevelopmentDeathAfterKeyAcceptance();
	void RunDevelopmentDataAcceptance();
#endif

private:
	void InitializePhase4();
	void RetryDiscoveryIfNeeded();
	void FindProductionArtAndCreateAnchors();
	void BindProductionInput();
	void UnbindProductionInput();
	void HandleInteractInput();
	void HandleEntered(ESOTMPhase4InteractableKind Kind, AActor* Actor);
	void HandleExited(ESOTMPhase4InteractableKind Kind, AActor* Actor);
	void RefreshPrompt();
	void InteractWithChest();
	void InteractWithGate();
	void BeginChestPresentation(bool bRestoreImmediately);
	void UpdateChestPresentation();
	void BeginGatePresentation(bool bRestoreImmediately);
	void UpdateGatePresentation();
	void FinishGatePresentation();
	void ActivateIsabelEncounter();
	void ShowDemoComplete();
	UFUNCTION()
	void HideDemoComplete();

	UFUNCTION()
	void HandlePlayerUnavailable(AActor* PlayerActor);

	UFUNCTION()
	void HandlePlayerRespawned(AActor* PlayerActor);

	UFUNCTION()
	void RetryDiscoveryTimerElapsed();

public:
	// Called by external system when Isabel is defeated
	UFUNCTION(BlueprintCallable, Category="SOTM|Demo|Phase 4")
	void OnIsabelDefeated();

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerStateSubsystem> PlayerState;

	UPROPERTY(Transient)
	TObjectPtr<USOTMObjectiveSubsystem> Objectives;

	UPROPERTY(Transient)
	TObjectPtr<AStaticMeshActor> ChestArt;

	UPROPERTY(Transient)
	TObjectPtr<AStaticMeshActor> GateArt;

	UPROPERTY(Transient)
	TObjectPtr<AStaticMeshActor> KeyPresentation;

	UPROPERTY(Transient)
	TObjectPtr<ASOTMPhase4Interactable> ChestAnchor;

	UPROPERTY(Transient)
	TObjectPtr<ASOTMPhase4Interactable> GateAnchor;

	UPROPERTY(Transient)
	TObjectPtr<USOTMDemoCompleteWidget> DemoCompleteWidget;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractInputAction;

	TWeakObjectPtr<UEnhancedInputComponent> BoundEnhancedInput;
	uint32 InteractBindingHandle = 0;
	bool bNearChest = false;
	bool bNearGate = false;
	bool bGateAnimationRunning = false;
	bool bDemoInputLockHeld = false;
	float ChestAnimationAlpha = 0.0f;
	float GateAnimationAlpha = 0.0f;
	FTransform ChestClosedTransform = FTransform::Identity;
	FTransform GateClosedTransform = FTransform::Identity;
	FTimerHandle InitializeTimer;
	FTimerHandle RetryDiscoveryTimer;
	FTimerHandle ChestAnimationTimer;
	FTimerHandle GateAnimationTimer;
	FTimerHandle DemoCompleteTimer;
};
