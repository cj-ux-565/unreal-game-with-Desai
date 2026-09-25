#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SOTMDemoPhase2WorldSubsystem.generated.h"

class AActor;
class ACameraActor;
class ASOTMCousinAIController;
class ASOTMCousinCharacter;
class APlayerController;
class UAudioComponent;
class UGameViewportClient;
class USOTMPlayerStateSubsystem;
class ASOTMCoinPickup;
class USOTMIngameUIWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTMCousinWarningSignature, bool, bVisible);

/**
 * CH1-only Phase 2 orchestration: activates the objective, replaces legacy
 * Forest Isabel copies with reusable Cousins, and globally gates lethal catch
 * cinematics so one encounter can consume only one life.
 */
UCLASS()
class SOTM1_API USOTMDemoPhase2WorldSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void NotifyCousinDetected(ASOTMCousinAIController* Controller);
	void NotifyCousinEncounterEnded(ASOTMCousinAIController* Controller);
	bool TryStartCousinCatch(ASOTMCousinCharacter* Cousin, AActor* PlayerActor);

	UFUNCTION(BlueprintPure, Category="SOTM|Cousin")
	bool IsCousinCatchActive() const { return bCatchActive; }

	UPROPERTY(BlueprintAssignable, Category="SOTM|Cousin|Events")
	FSOTMCousinWarningSignature OnCousinWarningChanged;

private:
	static FName GetMapPackageName(const UWorld* World);
	void InitializeForestPhase2();
	void EnsureForestGameplayHUD();
	void DisableLegacyForestEnemies(TArray<FTransform>& OutSpawnTransforms);
	void SpawnProductionCousins(const TArray<FTransform>& CandidateTransforms);
	void NormalizeForestAudioMix();
	void ScheduleNextCousinWhisper();
	void PlayOccasionalCousinWhisper();
	void PlayTemporaryCousinVoice(const TCHAR* SoundPath, const FText& Line,
		const FVector& Location, float Volume);
	void FindTeddyDialogueActor();
	void CheckTeddyProximity();
	void PlayTemporaryTeddyVoice();
	void CreateTeddySubtitleOverlay();
	void RemoveTeddySubtitleOverlay();
	void CreateCousinSubtitleOverlay();
	void RemoveCousinSubtitleOverlay();
	void SuspendAllCousins();
	void ResetAllCousins();
	void HideCousinWarning();
	void PresentCatchImpact();
	void FinishCatchPresentation();
	void AbortCatch(const TCHAR* Reason);
	void CreateCatchCamera(ASOTMCousinCharacter* Cousin, AActor* PlayerActor);
	void RestorePlayerCamera();
	void DestroyCatchCamera();

#if !UE_BUILD_SHIPPING
	void BeginDevelopmentAcceptanceRoute();
	void BeginDevelopmentPersistenceCheck();
	void BeginDevelopmentCompletionCheck();
	void BeginDevelopmentGameOverCheck();
	void CollectNextDevelopmentCoin();
	void PositionCousinForDevelopmentEncounter();
	void LogDevelopmentAcceptanceState(const TCHAR* Label) const;
	void CaptureDevelopmentEvidence(const FString& Label) const;
#endif

	UFUNCTION()
	void HandlePlayerDeathStarted(AActor* PlayerActor);

	UFUNCTION()
	void HandlePlayerRespawned(AActor* PlayerActor);

	UFUNCTION()
	void HandleGameOver();

	UFUNCTION()
	void HandleTemporaryCousinVoiceFinished();

	UFUNCTION()
	void HandleTemporaryTeddyVoiceFinished();

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerStateSubsystem> PlayerState;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASOTMCousinCharacter>> SpawnedCousins;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> CatchCamera;

	/** Set only when Phase 2 had to create the existing production HUD itself. */
	UPROPERTY(Transient)
	TObjectPtr<USOTMIngameUIWidget> Phase2CreatedHUD;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveCousinVoice;

	/** Existing CH1 Teddy #2 (Anim_HorrorBear_Idle4); Teddy #1 is never referenced. */
	TWeakObjectPtr<AActor> TeddyDialogueActor;
	/** One-shot per run: Teddy #2 speaks once, then never again. */
	bool bTeddyDialoguePlayed = false;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveTeddyVoice;

	TWeakObjectPtr<UGameViewportClient> TeddySubtitleViewport;
	TSharedPtr<class SWidget> TeddySubtitleRoot;
	TSharedPtr<class STextBlock> TeddySubtitleSpeakerText;
	TSharedPtr<class STextBlock> TeddySubtitleLineText;

	TWeakObjectPtr<UGameViewportClient> CousinSubtitleViewport;
	TSharedPtr<class SWidget> CousinSubtitleRoot;
	TSharedPtr<class STextBlock> CousinSubtitleSpeakerText;
	TSharedPtr<class STextBlock> CousinSubtitleLineText;

	TWeakObjectPtr<ASOTMCousinCharacter> CatchingCousin;
	TWeakObjectPtr<AActor> CaughtPlayer;
	TSet<TWeakObjectPtr<ASOTMCousinAIController>> WarnedControllers;
	bool bCatchActive = false;
	bool bCatchImpactApplied = false;
	bool bJumpScareLockHeld = false;

	FTimerHandle InitializeTimer;
	FTimerHandle WarningTimer;
	FTimerHandle CatchImpactTimer;
	FTimerHandle TeddyProximityTimer;
	FTimerHandle CatchFinishTimer;
	FTimerHandle CameraDestroyTimer;
	FTimerHandle CousinResetTimer;
	FTimerHandle CousinWhisperTimer;

#if !UE_BUILD_SHIPPING
	TArray<TWeakObjectPtr<ASOTMCoinPickup>> DevelopmentCoinRoute;
	int32 DevelopmentCoinRouteIndex = 0;
	bool bDevelopmentAcceptanceCatchObserved = false;
	FTimerHandle DevelopmentCoinTimer;
#endif
};
