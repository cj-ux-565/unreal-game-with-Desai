#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/SOTMIsabelAITypes.h"
#include "SOTMIsabelAIController.generated.h"

class ASOTMIsabelPatrolPoint;
class ACameraActor;
class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UAnimMontage;
class UAudioComponent;
class UCanvas;
class USoundBase;
class UUserWidget;

enum class ESOTMJumpScareCinematicPhase : uint8
{
	None,
	Catch,
	Anticipation,
	Impact,
	Recovery
};

/** Phase 1 state machine extended with the Phase 2 normal melee attack. */
UCLASS(Blueprintable)
class SOTM1_API ASOTMIsabelAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASOTMIsabelAIController();

	// Health and damage handling
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category="SOTM|Isabel|Health")
	void SetIsFinalBoss(bool bIsFinal) { bIsFinalIsabel = bIsFinal; }

	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Health")
	bool GetIsFinalBoss() const { return bIsFinalIsabel; }

	UFUNCTION(BlueprintCallable, Category="SOTM|Isabel|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category="SOTM|Isabel|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category="SOTM|Isabel|Health")
	void SetMaxHealth(float NewMaxHealth) { MaxHealth = NewMaxHealth; CurrentHealth = MaxHealth; }

	// Called by Phase4 subsystem to initialize boss encounter behavior
	UFUNCTION(BlueprintCallable, Category="SOTM|Isabel|Boss")
	void InitializeBossEncounter(AActor* PlayerTarget);

	// Called by BP_AI Blueprint EventGraph to check if legacy Blackboard should be used
	// Returns true if this is the final Isabel boss and should NOT use legacy behavior
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Boss")
	bool ShouldDisableLegacyBehavior() const { return bIsFinalIsabel; }

	// Alternative check - BP_AI can call this to see if it should skip its EventGraph
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Boss")
	bool IsIsabelBoss() const { return bIsFinalIsabel; }

protected:
	UFUNCTION(BlueprintCallable, Category="SOTM|Isabel|Health")
	void HandleIsabelDefeated();

private:
	void NotifySubsystemOfDefeat();
	void SyncIsabelBossHUD(bool bVisible);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") ESOTMIsabelAIState GetCurrentState() const { return CurrentState; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") FVector GetLastKnownPlayerLocation() const { return LastKnownPlayerLocation; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") bool CanCurrentlySeePlayer() const { return bCanSeePlayer; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") bool HasValidPathRequest() const { return bLastPathRequestValid; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") float GetCurrentMovementSpeed() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") ASOTMIsabelPatrolPoint* GetCurrentPatrolPoint() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|AI") float GetSearchTimeRemaining() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") float GetAttackDistance() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") float GetAttackRange() const { return AttackRange; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") bool HasAttackLineOfSight() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") bool IsFacingAttackTarget() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") bool CanAttackNow() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") bool IsAttackInProgress() const { return bAttackInProgress; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") bool WasDamageAppliedThisAttack() const { return bDamageAppliedThisAttack; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") float GetAttackCooldownRemaining() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") float GetLastSuccessfulDamageTime() const { return LastSuccessfulDamageTime; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Attack") bool IsPlayerDeadOrRespawning() const;
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Jump Scare") bool HasJumpScarePlayedThisEncounter() const { return bJumpScarePlayedThisEncounter; }
	UFUNCTION(BlueprintPure, Category="SOTM|Isabel|Jump Scare") bool HasJumpScareReachedImpact() const { return bJumpScareImpactReached; }
	UFUNCTION(BlueprintCallable, Category="SOTM|Isabel|Debug", meta=(DevelopmentOnly))
	bool ForceDevelopmentCatchForCinematicTest();

	public:
	/** Called only by the dedicated Phase 2 animation notify. */
	void HandleAttackImpactNotify();
	/** Called only by the dedicated Phase 3 jump-scare impact notify. */
	void HandleJumpScareImpactNotify();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception") TObjectPtr<UAIPerceptionComponent> IsabelPerception;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception") TObjectPtr<UAISenseConfig_Sight> SightConfig;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception") TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception", meta=(ClampMin="100.0")) float SightRadius = 1800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception", meta=(ClampMin="100.0")) float LoseSightRadius = 2200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception", meta=(ClampMin="1.0", ClampMax="180.0")) float PeripheralVisionHalfAngle = 70.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception", meta=(ClampMin="0.0")) float SightMemorySeconds = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception") bool bEnableHearing = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Perception", meta=(EditCondition="bEnableHearing", ClampMin="100.0")) float HearingRange = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Patrol") FName PatrolRouteId = TEXT("Isabel_Mansion_Phase1");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Patrol") ESOTMPatrolTraversalMode PatrolTraversal = ESOTMPatrolTraversalMode::Loop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Movement", meta=(ClampMin="0.0")) float PatrolSpeed = 180.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Movement", meta=(ClampMin="0.0")) float InvestigateSpeed = 260.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Movement", meta=(ClampMin="0.0")) float ChaseSpeed = 480.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Movement", meta=(ClampMin="5.0")) float AcceptanceRadius = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Search", meta=(ClampMin="0.1")) float SearchDuration = 6.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Search", meta=(ClampMin="0.1")) float InvestigateDuration = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Search", meta=(ClampMin="0.0")) float SearchRadius = 350.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Chase", meta=(ClampMin="0.1")) float ChaseRepathInterval = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Chase", meta=(ClampMin="0.0")) float ChaseRepathDistance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack", meta=(ClampMin="25.0")) float AttackRange = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack", meta=(ClampMin="0.0")) float AttackExitMargin = 40.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack", meta=(ClampMin="0.1")) float AttackCooldown = 2.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack", meta=(ClampMin="0.0")) float AttackDamage = 25.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack", meta=(ClampMin="1.0", ClampMax="90.0")) float FacingToleranceDegrees = 18.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack", meta=(ClampMin="0.0")) float AttackWindUpSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack", meta=(ClampMin="0.0")) float AttackRecoverySeconds = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Attack") TSoftObjectPtr<UAnimMontage> NormalAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare") bool bEnableJumpScare = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare", meta=(ClampMin="0.0")) float JumpScareCameraBlendInSeconds = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare", meta=(ClampMin="0.0")) float JumpScareCameraBlendOutSeconds = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare", meta=(ClampMin="40.0")) float JumpScareCameraDistance = 70.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare") float JumpScareCameraSideOffset = 70.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare") float JumpScareFocusHeight = 88.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare", meta=(ClampMin="30.0", ClampMax="120.0")) float JumpScareCameraFOV = 60.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="30.0", ClampMax="120.0")) float JumpScareAnticipationFOV = 68.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="30.0", ClampMax="120.0")) float JumpScareImpactFOV = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="0.0", ClampMax="40.0")) float JumpScareCameraPushInDistance = 16.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="0.05", ClampMax="0.6")) float JumpScareImpactEffectSeconds = 0.24f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="0.0", ClampMax="1.0")) float JumpScarePeakVignette = 0.52f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="-2.0", ClampMax="0.0")) float JumpScarePeakExposureBias = -0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="0.0", ClampMax="3.0")) float JumpScarePeakChromaticAberration = 1.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="0.0", ClampMax="5.0")) float JumpScareImpactRotationAmplitude = 1.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare|Polish", meta=(ClampMin="0.0", ClampMax="10.0")) float JumpScareImpactLocationAmplitude = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Death Presentation", meta=(ClampMin="0.0", ClampMax="3.0")) float IsabelDeathAnimationLeadInSeconds = 1.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Death Presentation", meta=(ClampMin="0.05", ClampMax="1.0")) float IsabelDeathFadeSeconds = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Death Presentation", meta=(ClampMin="0.0", ClampMax="1.0")) float IsabelDeathFadeOpacity = 0.85f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare") FName JumpScareFocusBone = TEXT("head");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare", meta=(ClampMin="1.0")) float JumpScareCameraTrackingSpeed = 12.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare") TSoftObjectPtr<UAnimMontage> JumpScareMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Jump Scare") TSoftObjectPtr<USoundBase> JumpScareSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Debug") bool bDrawDevelopmentDebug = true;

	// Health properties - instance editable for the final boss
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTM|Isabel|Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Health")
	float CurrentHealth = 100.0f;

	// Whether this is the final Isabel boss (triggers Chapter 1 completion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTM|Isabel|Health")
	bool bIsFinalIsabel = false;

private:
	UFUNCTION() void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	UFUNCTION() void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	void RefreshPerceptionSettings();
	void RegisterPlayerAsPerceptionSource();
	void DiscoverPatrolRoute();
	void EvaluateState();
	void SetState(ESOTMIsabelAIState NewState, const TCHAR* Reason);
	void EnterPatrol();
	void MoveToCurrentPatrolPoint();
	void AdvancePatrolPoint();
	void BeginSearchAtLastKnownLocation();
	void BeginReturnToPatrol();
	void EvaluateChase();
	void EvaluateAttack();
	void RepathChaseIfNeeded(bool bForce);
	bool TryBeginAttack();
	void BeginAttack();
	void StartAttackMontage();
	void FinishAttack();
	void AbortAttack(const TCHAR* Reason);
	void FaceAttackTarget();
	void EnterAttackRotationMode();
	void RestoreLocomotionRotationMode();
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	bool ShouldBeginJumpScare(const AActor* Target) const;
	void BeginJumpScare(AActor* Target);
	void StartJumpScareMontage();
	void HandleJumpScareMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void FinishJumpScare();
	void ResumeAfterJumpScare();
	void AbortJumpScare(const TCHAR* Reason, bool bRestoreCameraImmediately);
	void ResetJumpScareEncounter(const TCHAR* Reason);
	bool CreateJumpScareCamera(AActor* Target);
	void UpdateJumpScareCameraTracking();
	void ApplyJumpScareCinematicEffects(float EffectStrength, float TargetFOV);
	void ClearJumpScareCinematicEffects();
	void BeginIsabelDeathTransition();
	void StartIsabelDeathFade();
	void ClearIsabelDeathTransition(bool bFadeBackToGameplay);
	void RestorePlayerCamera(bool bImmediate);
	void ReleaseJumpScareInputLock();
	UFUNCTION() void HandlePlayerRespawned(AActor* PlayerActor);
	void StartPatrolWait(float Duration);
	void SetMovementSpeed(float Speed) const;
	void ClearPlayerTarget(bool bSearchLastKnown);
	bool IsValidLivingPlayer(const AActor* Actor) const;
	bool IsSightStimulus(const FAIStimulus& Stimulus) const;
	bool IsHearingStimulus(const FAIStimulus& Stimulus) const;
	FString BuildDebugText() const;
	static const TCHAR* StateToString(ESOTMIsabelAIState State);

	UPROPERTY(Transient) ESOTMIsabelAIState CurrentState = ESOTMIsabelAIState::Idle;
	UPROPERTY(Transient) TWeakObjectPtr<AActor> CurrentTarget;
	UPROPERTY(Transient) TArray<TWeakObjectPtr<ASOTMIsabelPatrolPoint>> PatrolPoints;

	FVector LastKnownPlayerLocation = FVector::ZeroVector;
	FVector LastChaseRequestLocation = FVector::ZeroVector;
	int32 CurrentPatrolIndex = INDEX_NONE;
	int32 PatrolDirection = 1;
	int32 ConsecutivePathFailures = 0;
	bool bCanSeePlayer = false;
	bool bLastPathRequestValid = false;
	bool bWaitingAtPatrolPoint = false;
	bool bPlayerStimulusRegistered = false;
	bool bAttackInProgress = false;
	bool bDamageAppliedThisAttack = false;
	bool bAttackRotationModeActive = false;
	bool bJumpScareInProgress = false;
	bool bJumpScarePlayedThisEncounter = false;
	bool bJumpScareImpactReached = false;
	bool bJumpScareInputLockHeld = false;
	bool bJumpScareMontageStarted = false;
	bool bJumpScareSoundPlayed = false;
	bool bIsabelDeathFadeActive = false;
	bool bSavedOrientRotationToMovement = true;
	bool bSavedUseControllerDesiredRotation = false;
	bool bSavedUseControllerRotationYaw = false;
	float SearchEndTime = 0.0f;
	float NextSearchMoveTime = 0.0f;
	float NextChaseRepathTime = 0.0f;
	float NextAttackAllowedTime = 0.0f;
	float LastSuccessfulDamageTime = -1.0f;
	float JumpScareCinematicStartTime = 0.0f;
	float JumpScareImpactStartTime = -1.0f;
	FVector JumpScareCameraBaseLocation = FVector::ZeroVector;
	ESOTMJumpScareCinematicPhase JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::None;
	TWeakObjectPtr<AActor> JumpScareTarget;
	TWeakObjectPtr<ACameraActor> JumpScareCamera;
	TWeakObjectPtr<UAudioComponent> JumpScareAudioComponent;
	FTimerHandle EvaluationTimer;
	FTimerHandle PatrolWaitTimer;
	FTimerHandle AttackWindUpTimer;
	FTimerHandle AttackRecoveryTimer;
	FTimerHandle JumpScareStartTimer;
	FTimerHandle JumpScareResumeTimer;
	FTimerHandle JumpScareCameraReleaseTimer;
	FTimerHandle JumpScareCameraTrackingTimer;
	FTimerHandle IsabelDeathFadeTimer;

	// Screen-space boss health bar (WBP_IsabelHealth), owned via the viewport.
	// Created on Phase4 activation, removed on defeat; never exists pre-gate.
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> IsabelBossHUD;
};
