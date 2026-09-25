#include "AI/SOTMIsabelAIController.h"

#include "BrainComponent.h"
#include "AI/SOTMIsabelPatrolPoint.h"
#include "Demo/SOTMDemoPhase4WorldSubsystem.h"
#include "Objective/SOTMObjectiveSubsystem.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Sound/SoundBase.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMIsabelAI, Log, All);

namespace SOTMIsabelChaseFix
{
	// Chase progress guard state. File-local (not UPROPERTY) because this fix is
	// confined to SOTMIsabelAIController.cpp by design. Reset on every fresh
	// possession/encounter (see ResetChaseProgressGuard) so PIE re-runs start clean.
	float LastProgressDistance = -1.0f; // 3D distance to player when the last request was issued (<0 = no baseline)
	int32 StallCount = 0; // consecutive issued requests with no meaningful progress
	bool bPartialPathActive = false; // the last issued chase request produced a partial path
	bool bBlockerReported = false; // throttles the navigation-blocker log until progress resumes

	constexpr float ProgressEpsilon = 25.0f; // minimum distance closed to count as progress (conservative vs 480 u/s chase)
	constexpr int32 MaxStallRepaths = 4; // ~2s of zero progress before standing down (repath interval is 0.5s)
}

static void ResetChaseProgressGuard()
{
	SOTMIsabelChaseFix::LastProgressDistance = -1.0f;
	SOTMIsabelChaseFix::StallCount = 0;
	SOTMIsabelChaseFix::bPartialPathActive = false;
	SOTMIsabelChaseFix::bBlockerReported = false;
}

// Boss health bar sync: screen-space WBP_IsabelHealth owned via the viewport.
// Uses the existing CurrentHealth/MaxHealth only — no second health system.
// Created+shown on Phase4 activation, updated on damage, removed on defeat;
// it never exists before the gate opens.
void ASOTMIsabelAIController::SyncIsabelBossHUD(bool bVisible)
{
	if (!bVisible)
	{
		if (IsabelBossHUD)
		{
			IsabelBossHUD->RemoveFromParent();
			IsabelBossHUD = nullptr;
		}
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}
	if (!IsabelBossHUD)
	{
		if (UClass* HUDClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/AI/WBP_IsabelHealth.WBP_IsabelHealth_C")))
		{
			IsabelBossHUD = CreateWidget<UUserWidget>(PC, HUDClass);
			if (IsabelBossHUD)
			{
				IsabelBossHUD->AddToViewport(100);
			}
		}
	}
	if (!IsabelBossHUD)
	{
		return;
	}
	if (!IsabelBossHUD->IsInViewport())
	{
		IsabelBossHUD->AddToViewport(100);
	}
	if (UWidgetTree* Tree = IsabelBossHUD->WidgetTree)
	{
		// Horror presentation only: ominous bone name, muted blood-red fill.
		// Health values and visibility lifecycle are unchanged.
		if (UTextBlock* BossName = Tree->FindWidget<UTextBlock>(TEXT("BossNameLabel")))
		{
			BossName->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.87f, 0.82f, 1.0f)));
		}
		if (UProgressBar* Fill = Tree->FindWidget<UProgressBar>(TEXT("BossHealthFill")))
		{
			const float Max = GetMaxHealth();
			Fill->SetPercent(Max > 0.0f ? FMath::Clamp(GetCurrentHealth() / Max, 0.0f, 1.0f) : 0.0f);
			Fill->SetFillColorAndOpacity(FLinearColor(0.48f, 0.07f, 0.09f, 1.0f));
		}
		if (UProgressBar* AltFill = Tree->FindWidget<UProgressBar>(TEXT("HealthFill")))
		{
			AltFill->SetFillColorAndOpacity(FLinearColor(0.48f, 0.07f, 0.09f, 1.0f));
		}
	}
}

// Throttled navigation-blocker report: Isabel location, player location, whether the
// path is partial, the reachable endpoint, and what level change would unblock pursuit.
// Chase stays alive (CurrentTarget kept); it resumes automatically when the player relocates.
static void ReportIsabelChaseBlocked(ASOTMIsabelAIController* Isabel, const AActor* PlayerTarget, float DistToPlayer, bool bPartialPath)
{
	if (!Isabel || SOTMIsabelChaseFix::bBlockerReported)
	{
		return;
	}
	SOTMIsabelChaseFix::bBlockerReported = true;
	const FVector IsabelLoc = Isabel->GetPawn() ? Isabel->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const FVector PlayerLoc = PlayerTarget ? PlayerTarget->GetActorLocation() : FVector::ZeroVector;
	FVector ReachableEnd = FVector::ZeroVector;
	if (const UPathFollowingComponent* PathComp = Isabel->GetPathFollowingComponent())
	{
		ReachableEnd = PathComp->GetPathDestination();
	}
	UE_LOG(LogSOTMIsabelAI, Warning,
		TEXT("ISABEL CHASE BLOCKED: partial=%d stalls=%d isabel=%s player=%s dist=%.0f reachable_end=%s. NavMesh between these areas is disconnected; connect navigation (volumes/ramps/links) to allow physical pursuit. Chase kept alive and resumes when the player relocates."),
		bPartialPath ? 1 : 0, SOTMIsabelChaseFix::StallCount, *IsabelLoc.ToString(), *PlayerLoc.ToString(), DistToPlayer, *ReachableEnd.ToString());
}

ASOTMIsabelAIController::ASOTMIsabelAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	NormalAttackMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/AI/AM_Isabel_NormalAttack_Phase2.AM_Isabel_NormalAttack_Phase2")));
	JumpScareMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/AI/AM_Isabel_JumpScare_Phase3.AM_Isabel_JumpScare_Phase3")));
	JumpScareSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/AI/Nightmare_scream_jumpscare_SFX.Nightmare_scream_jumpscare_SFX")));

	IsabelPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("IsabelPerception"));
	SetPerceptionComponent(*IsabelPerception);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	IsabelPerception->ConfigureSense(*SightConfig);
	IsabelPerception->SetDominantSense(SightConfig->GetSenseImplementation());

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	IsabelPerception->ConfigureSense(*HearingConfig);
}

bool ASOTMIsabelAIController::ForceDevelopmentCatchForCinematicTest()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AActor* Target = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!IsValidLivingPlayer(Target) || IsPlayerDeadOrRespawning() ||
		GetPawn() == nullptr || FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation()) > AttackRange ||
		!LineOfSightTo(Target))
	{
		UE_LOG(LogSOTMIsabelAI, Warning,
			TEXT("Development cinematic catch rejected: requires living player, valid range and clear line of sight"));
		return false;
	}

	CurrentTarget = Target;
	JumpScareTarget = Target;
	LastKnownPlayerLocation = Target->GetActorLocation();
	bCanSeePlayer = true;
	SetState(ESOTMIsabelAIState::Chase, TEXT("development-only cinematic verification catch"));
	FaceAttackTarget();
	BeginJumpScare(Target);
	return bJumpScareInProgress;
#endif
}

void ASOTMIsabelAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Fresh possession: the chase progress guard must not inherit state from a previous run.
	ResetChaseProgressGuard();

	// Dormant until Phase4 activation: no boss HUD exists before the gate opens.
	SyncIsabelBossHUD(false);

	// Pre-gate concealment: the dedicated Chapter 1 boss pawn is placed in the level
	// from the start but must stay invisible until Phase4 gate activation. AI dormancy
	// is already enforced by the bIsFinalIsabel perception guard; this only hides the
	// placed pawn and removes its collision so the player can neither see her nor
	// trigger any premature encounter. Generic Isabel pawns are never affected.
	// (The Phase4 spawn path sets bIsFinalIsabel BEFORE Possess, so it stays visible.)
	if (!bIsFinalIsabel && IsDedicatedIsabelBossPawn(InPawn))
	{
		SetDedicatedBossStageHidden(true);
		StopMovement();
	}

	UE_LOG(LogSOTMIsabelAI, Display, TEXT("=== OnPossess START for %s ==="), *GetNameSafe(InPawn));

	// CRITICAL: Ensure we have a valid BrainComponent (Blackboard)
	// This is needed because BP_AI Blueprint's EventGraph calls GetBlackboard
	// Without this, BP_AI33 will produce "Accessed None" errors
	if (BrainComponent)
	{
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("BrainComponent already valid"));
	}
	else
	{
		UE_LOG(LogSOTMIsabelAI, Warning, TEXT("BrainComponent is null - BP_AI Blueprint may fail"));
	}

	RefreshPerceptionSettings();
	IsabelPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	ReceiveMoveCompleted.AddUniqueDynamic(this, &ThisClass::HandleMoveCompleted);
	BindPawnDamageForwarding(InPawn);

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (USOTMPlayerStateSubsystem* State = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>())
		{
			State->OnPlayerRespawned.AddUniqueDynamic(this, &ThisClass::HandlePlayerRespawned);
		}
	}
	
	DiscoverPatrolRoute();
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("Patrol points found: %d"), PatrolPoints.Num());

	// Register player as perception source immediately
	RegisterPlayerAsPerceptionSource();
	
	// Start evaluation timer
	GetWorldTimerManager().SetTimer(EvaluationTimer, this, &ThisClass::EvaluateState, 0.2f, true, 0.1f);
	
	// For boss encounter (bIsFinalIsabel): directly target the player immediately
	if (bIsFinalIsabel)
	{
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		AActor* PlayerActor = PlayerController ? PlayerController->GetPawn() : nullptr;
		
		if (PlayerActor && IsValidLivingPlayer(PlayerActor))
		{
			CurrentTarget = PlayerActor;
			bCanSeePlayer = true;
			LastKnownPlayerLocation = PlayerActor->GetActorLocation();
			
			// Start chase immediately
			SetMovementSpeed(ChaseSpeed);
			SetState(ESOTMIsabelAIState::Chase, TEXT("boss encounter - player acquired"));
			EvaluateChase();

			// Final-boss combat balance: full health at encounter start, before HUD sync.
			SetMaxHealth(350.0f);

			// Final boss possessed here (fresh spawn path): show the existing boss
			// HUD now so it is visible during the fight-start reveal. Same cached
			// instance; health sync and defeat removal are unchanged.
			SyncIsabelBossHUD(true);
			
			UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL TARGET: %s"), *PlayerActor->GetName());
			UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL STATE: Chase"));
		}
		else
		{
			UE_LOG(LogSOTMIsabelAI, Warning, TEXT("BOSS ENCOUNTER: no valid player found, entering patrol"));
			EnterPatrol();
		}
	}
	else
	{
		// Normal Isabel: enter patrol behavior
		EnterPatrol();
	}

	UE_LOG(LogSOTMIsabelAI, Display, TEXT("=== OnPossess END ==="));
}

bool ASOTMIsabelAIController::IsDedicatedIsabelBossPawn(const APawn* PawnActor) const
{
	if (!PawnActor)
	{
		return false;
	}
	// Same definition of "the dedicated boss" the Phase4 subsystem uses to find her:
	// actor label IsabelBoss, or any BP_IsabelAI instance. The class-name check also
	// survives cooked builds, where editor actor labels are unavailable.
	return PawnActor->GetClass()->GetName().Contains(TEXT("BP_IsabelAI"))
		|| PawnActor->GetActorLabel(false) == TEXT("IsabelBoss");
}

void ASOTMIsabelAIController::SetDedicatedBossStageHidden(bool bShouldHide)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}
	ControlledPawn->SetActorHiddenInGame(bShouldHide);
	ControlledPawn->SetActorEnableCollision(!bShouldHide);
}

void ASOTMIsabelAIController::InitializeBossEncounter(AActor* PlayerTarget)
{
	if (!PlayerTarget)
	{
		UE_LOG(LogSOTMIsabelAI, Warning, TEXT("InitializeBossEncounter: No player target"));
		return;
	}

	UE_LOG(LogSOTMIsabelAI, Display, TEXT("=== InitializeBossEncounter START ==="));

	// Fresh boss chase: reset the progress guard so earlier movement is never misread as a stall.
	ResetChaseProgressGuard();
	
	// Ensure perception is set up
	RefreshPerceptionSettings();
	
	// Register perception if not already done
	if (!bPlayerStimulusRegistered)
	{
		RegisterPlayerAsPerceptionSource();
	}
	
	// Start evaluation timer if not already running
	if (!GetWorldTimerManager().IsTimerActive(EvaluationTimer))
	{
		GetWorldTimerManager().SetTimer(EvaluationTimer, this, &ThisClass::EvaluateState, 0.2f, true, 0.1f);
	}
	
	// Directly target the player
	CurrentTarget = PlayerTarget;
	bCanSeePlayer = true;
	LastKnownPlayerLocation = PlayerTarget->GetActorLocation();
	
	// Start chase immediately
	SetMovementSpeed(ChaseSpeed);
	SetState(ESOTMIsabelAIState::Chase, TEXT("boss encounter initialized"));
	
	// Force immediate movement request
	AActor* Target = CurrentTarget.Get();
	if (Target)
	{
		float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation());
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL DISTANCE TO PLAYER: %.0f"), DistToPlayer);
		
		const EPathFollowingRequestResult::Type Result = MoveToActor(Target, AcceptanceRadius, true, true, true, nullptr, true);
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL MOVE RESULT: %d"), (int32)Result);
	}
	
	EvaluateChase();
	
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL TARGET: %s"), *PlayerTarget->GetName());
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL STATE: Chase"));
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL MOVEMENT STARTED"));

	// Phase4 activation: the dedicated boss becomes visible and collidable again,
	// restoring normal chase/attack interaction with the world.
	SetDedicatedBossStageHidden(false);

	// Final-boss combat balance: full health at encounter start, before HUD sync.
	SetMaxHealth(350.0f);

	// Phase4 activation: reveal the screen-space boss health bar with current health.
	SyncIsabelBossHUD(true);

	UE_LOG(LogSOTMIsabelAI, Display, TEXT("=== InitializeBossEncounter END ==="));
}

void ASOTMIsabelAIController::OnUnPossess()
{
	AbortAttack(TEXT("controller unpossessed"));
	ClearIsabelDeathTransition(false);
	GetWorldTimerManager().ClearTimer(EvaluationTimer);
	GetWorldTimerManager().ClearTimer(PatrolWaitTimer);
	AbortJumpScare(TEXT("controller unpossessed"), true);
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (USOTMPlayerStateSubsystem* State = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>())
		{
			State->OnPlayerRespawned.RemoveDynamic(this, &ThisClass::HandlePlayerRespawned);
		}
	}
	IsabelPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	ReceiveMoveCompleted.RemoveDynamic(this, &ThisClass::HandleMoveCompleted);
	UnbindPawnDamageForwarding();
	CurrentTarget.Reset();
	PatrolPoints.Reset();
	Super::OnUnPossess();
}

float ASOTMIsabelAIController::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.0f && CurrentHealth > 0.0f)
	{
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel took damage: %.1f / %.1f"), CurrentHealth, MaxHealth);

		// Immediate health bar update from the existing health value (no second system).
		// Gated on activation so pre-gate or post-defeat damage can never reveal the HUD.
		if (bIsFinalIsabel && CurrentHealth > 0.0f)
		{
			SyncIsabelBossHUD(true);
		}

		if (CurrentHealth <= 0.0f)
		{
			HandleIsabelDefeated();
		}
	}
	return ActualDamage;
}

void ASOTMIsabelAIController::HandleIsabelDefeated()
{
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel defeated! bIsFinalIsabel=%s"), bIsabelDeathFadeActive ? TEXT("true") : TEXT("false"));

	// Defeat: remove the boss health bar as part of the existing defeat flow.
	SyncIsabelBossHUD(false);

	// Stop AI behavior
	AbortAttack(TEXT("Isabel defeated"));
	ClearIsabelDeathTransition(false);
	GetWorldTimerManager().ClearAllTimersForObject(this);
	
	// Disable perception to stop chasing
	if (IsabelPerception)
	{
		IsabelPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
		IsabelPerception->SetSenseEnabled(UAISense_Hearing::StaticClass(), false);
	}

	// Notify subsystem if this is the final boss
	if (bIsFinalIsabel)
	{
		NotifySubsystemOfDefeat();
	}

	// Post-defeat presentation only: hide the defeated pawn and remove its
	// collision. Actor is kept alive; no health/objective/save logic touched.
	SetDedicatedBossStageHidden(true);
}

void ASOTMIsabelAIController::NotifySubsystemOfDefeat()
{
	if (UWorld* World = GetWorld())
	{
		// First complete the objective, then trigger the demo completion
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (USOTMObjectiveSubsystem* Objectives = GameInstance->GetSubsystem<USOTMObjectiveSubsystem>())
			{
				Objectives->TryCompleteDefeatIsabel();
			}
		}
		
		// Then call the Phase 4 subsystem to show demo complete
		if (USOTMDemoPhase4WorldSubsystem* Phase4 = World->GetSubsystem<USOTMDemoPhase4WorldSubsystem>())
		{
			Phase4->OnIsabelDefeated();
			UE_LOG(LogSOTMIsabelAI, Display, TEXT("Notified Phase4 subsystem of Isabel defeat"));
		}
		else
		{
			UE_LOG(LogSOTMIsabelAI, Warning, TEXT("Could not find Phase4 subsystem to notify of Isabel defeat"));
		}
	}
}

void ASOTMIsabelAIController::BindPawnDamageForwarding(APawn* TargetPawn)
{
	UnbindPawnDamageForwarding();
	if (IsValid(TargetPawn))
	{
		TargetPawn->OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HandlePawnAnyDamage);
		DamageForwardPawn = TargetPawn;
	}
}

void ASOTMIsabelAIController::UnbindPawnDamageForwarding()
{
	if (APawn* TrackedPawn = DamageForwardPawn.Get())
	{
		TrackedPawn->OnTakeAnyDamage.RemoveDynamic(this, &ThisClass::HandlePawnAnyDamage);
	}
	DamageForwardPawn.Reset();
}

void ASOTMIsabelAIController::HandlePawnAnyDamage(
	AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	// Bridge only: dedicated final boss, own controlled pawn, real damage.
	// Health subtraction, HUD sync and defeat stay inside TakeDamage().
	APawn* ControlledPawn = GetPawn();
	if (!bIsFinalIsabel || !ControlledPawn || DamagedActor != ControlledPawn)
	{
		return;
	}
	if (!IsDedicatedIsabelBossPawn(ControlledPawn))
	{
		return;
	}
	if (Damage <= 0.0f || CurrentHealth <= 0.0f)
	{
		return;
	}
	FDamageEvent DamageEvent;
	DamageEvent.DamageTypeClass = DamageType ? DamageType->GetClass() : nullptr;
	TakeDamage(Damage, DamageEvent, InstigatedBy, DamageCauser);
}

void ASOTMIsabelAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbortJumpScare(TEXT("controller ending play"), true);
	ClearIsabelDeathTransition(false);
	UnbindPawnDamageForwarding();
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

void ASOTMIsabelAIController::RefreshPerceptionSettings()
{
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(SightRadius, LoseSightRadius);
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(SightMemorySeconds);
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(InvestigateDuration);
	IsabelPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), true);
	IsabelPerception->SetSenseEnabled(UAISense_Hearing::StaticClass(), bEnableHearing);
	IsabelPerception->RequestStimuliListenerUpdate();
}

void ASOTMIsabelAIController::RegisterPlayerAsPerceptionSource()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	AActor* PlayerActor = PlayerController ? PlayerController->GetPawn() : nullptr;
	
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("RegisterPlayerAsPerceptionSource: PlayerActor=%s"), *GetNameSafe(PlayerActor));
	
	if (PlayerActor)
	{
		bPlayerStimulusRegistered = UAIPerceptionSystem::RegisterPerceptionStimuliSource(
			this, UAISense_Sight::StaticClass(), PlayerActor);
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("Registered sight stimulus: %s"), bPlayerStimulusRegistered ? TEXT("success") : TEXT("failed"));
		
		if (bEnableHearing)
		{
			UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Hearing::StaticClass(), PlayerActor);
		}
	}
	else
	{
		UE_LOG(LogSOTMIsabelAI, Warning, TEXT("No player actor found for perception registration!"));
	}
}

void ASOTMIsabelAIController::DiscoverPatrolRoute()
{
	PatrolPoints.Reset();
	for (TActorIterator<ASOTMIsabelPatrolPoint> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->RouteId == PatrolRouteId)
		{
			PatrolPoints.Add(*It);
		}
	}

	PatrolPoints.Sort([](const TWeakObjectPtr<ASOTMIsabelPatrolPoint>& Left, const TWeakObjectPtr<ASOTMIsabelPatrolPoint>& Right)
	{
		if (!Left.IsValid()) return false;
		if (!Right.IsValid()) return true;
		return Left->Order < Right->Order;
	});

	CurrentPatrolIndex = PatrolPoints.IsEmpty() ? INDEX_NONE : 0;
	PatrolDirection = 1;
	ConsecutivePathFailures = 0;
}

void ASOTMIsabelAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || (!IsSightStimulus(Stimulus) && !IsHearingStimulus(Stimulus)))
	{
		return;
	}
	// Dormancy: the dedicated Chapter 1 boss must stay completely inactive until
	// Phase4 gate activation. Only Phase4 sets bIsFinalIsabel (SetIsFinalBoss), so it
	// is the activation flag: pre-gate stimuli are ignored, keeping her out of
	// Chase/Attack/JumpScare until the gate opens. Post-gate behavior is unchanged.
	if (!bIsFinalIsabel)
	{
		return;
	}
	// Perception updates must not tear down the active catch cinematic while its
	// camera, animation, and input lock own the presentation.
	if (CurrentState == ESOTMIsabelAIState::JumpScare)
	{
		return;
	}

	if (IsSightStimulus(Stimulus))
	{
		if (Stimulus.WasSuccessfullySensed() && IsValidLivingPlayer(Actor))
		{
			CurrentTarget = Actor;
			bCanSeePlayer = true;
			LastKnownPlayerLocation = Actor->GetActorLocation();
			if (CurrentState != ESOTMIsabelAIState::Attack)
			{
				SetMovementSpeed(ChaseSpeed);
				SetState(ESOTMIsabelAIState::Chase, TEXT("valid player acquired by sight"));
				EvaluateChase();
			}
		}
		else if (CurrentTarget.Get() == Actor)
		{
			LastKnownPlayerLocation = Stimulus.StimulusLocation.IsNearlyZero()
				? Actor->GetActorLocation()
				: Stimulus.StimulusLocation;
			AbortAttack(TEXT("line of sight lost"));
			ClearPlayerTarget(true);
		}
		return;
	}

	if (bEnableHearing && Stimulus.WasSuccessfullySensed() && IsValidLivingPlayer(Actor) && CurrentState != ESOTMIsabelAIState::Chase)
	{
		LastKnownPlayerLocation = Stimulus.StimulusLocation;
		SetMovementSpeed(InvestigateSpeed);
		SetState(ESOTMIsabelAIState::Investigate, TEXT("valid player noise heard"));
		SearchEndTime = GetWorld()->GetTimeSeconds() + InvestigateDuration;
		const EPathFollowingRequestResult::Type Result = MoveToLocation(LastKnownPlayerLocation, AcceptanceRadius, true, true, true, false);
		bLastPathRequestValid = Result != EPathFollowingRequestResult::Failed;
	}
}

void ASOTMIsabelAIController::EvaluateState()
{
	if (!GetPawn())
	{
		return;
	}

	if (!bPlayerStimulusRegistered)
	{
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		AActor* PlayerActor = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (PlayerActor)
		{
			bPlayerStimulusRegistered = UAIPerceptionSystem::RegisterPerceptionStimuliSource(
				this, UAISense_Sight::StaticClass(), PlayerActor);
			if (bEnableHearing)
			{
				UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Hearing::StaticClass(), PlayerActor);
			}
		}
	}

	if (CurrentState != ESOTMIsabelAIState::JumpScare && CurrentTarget.IsValid() && !IsValidLivingPlayer(CurrentTarget.Get()))
	{
		AbortAttack(TEXT("player dead, respawning or unavailable"));
		ClearPlayerTarget(false);
		BeginReturnToPatrol();
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	switch (CurrentState)
	{
	case ESOTMIsabelAIState::Chase:
		EvaluateChase();
		break;
	case ESOTMIsabelAIState::Attack:
		EvaluateAttack();
		break;
	case ESOTMIsabelAIState::Investigate:
		if (Now >= SearchEndTime)
		{
			BeginReturnToPatrol();
		}
		break;
	case ESOTMIsabelAIState::JumpScare:
		// Montage callbacks own the cinematic; no polling or Event Tick is required.
		break;
	case ESOTMIsabelAIState::SearchLastKnown:
		if (Now >= SearchEndTime)
		{
			BeginReturnToPatrol();
		}
		else if (Now >= NextSearchMoveTime && GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			NextSearchMoveTime = Now + 1.25f;
			FNavLocation SearchLocation;
			if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
				Nav && Nav->GetRandomReachablePointInRadius(LastKnownPlayerLocation, SearchRadius, SearchLocation))
			{
				const EPathFollowingRequestResult::Type Result = MoveToLocation(SearchLocation.Location, AcceptanceRadius, true, true, true, false);
				bLastPathRequestValid = Result != EPathFollowingRequestResult::Failed;
			}
		}
		break;
	default:
		break;
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDevelopmentDebug && GetPawn() && CurrentState != ESOTMIsabelAIState::JumpScare)
	{
		const FVector TextLocation = GetPawn()->GetActorLocation() + FVector(0.0f, 0.0f, 130.0f);
		DrawDebugString(GetWorld(), TextLocation, BuildDebugText(), nullptr, FColor::Purple, 0.22f, false, 1.0f);
		if (!LastKnownPlayerLocation.IsNearlyZero())
		{
			DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), LastKnownPlayerLocation, FColor::Yellow, false, 0.22f, 0, 1.0f);
		}
	}
#endif
}

void ASOTMIsabelAIController::SetState(ESOTMIsabelAIState NewState, const TCHAR* Reason)
{
	if (CurrentState == NewState)
	{
		return;
	}
	if (CurrentState == ESOTMIsabelAIState::ReturnToPatrol ||
		(CurrentState == ESOTMIsabelAIState::Patrol && bWaitingAtPatrolPoint))
	{
		GetWorldTimerManager().ClearTimer(PatrolWaitTimer);
		bWaitingAtPatrolPoint = false;
	}
	const ESOTMIsabelAIState PreviousState = CurrentState;
	CurrentState = NewState;
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel state %s -> %s (%s)"), StateToString(PreviousState), StateToString(NewState), Reason);
}

void ASOTMIsabelAIController::EnterPatrol()
{
	ResetJumpScareEncounter(TEXT("patrol entered"));
	StopMovement();
	bWaitingAtPatrolPoint = false;
	SetMovementSpeed(PatrolSpeed);
	if (PatrolPoints.IsEmpty())
	{
		SetState(ESOTMIsabelAIState::Idle, TEXT("no valid patrol points"));
		bLastPathRequestValid = false;
		return;
	}
	SetState(ESOTMIsabelAIState::Patrol, TEXT("patrol route available"));
	MoveToCurrentPatrolPoint();
}

void ASOTMIsabelAIController::MoveToCurrentPatrolPoint()
{
	ASOTMIsabelPatrolPoint* Point = GetCurrentPatrolPoint();
	if (!Point)
	{
		++ConsecutivePathFailures;
		if (ConsecutivePathFailures >= PatrolPoints.Num())
		{
			SetState(ESOTMIsabelAIState::Idle, TEXT("patrol route contains no valid points"));
			bLastPathRequestValid = false;
			return;
		}
		AdvancePatrolPoint();
		return;
	}

	const EPathFollowingRequestResult::Type Result = MoveToActor(Point, AcceptanceRadius, true, true, true, nullptr, true);
	bLastPathRequestValid = Result != EPathFollowingRequestResult::Failed;
	if (Result == EPathFollowingRequestResult::Failed)
	{
		++ConsecutivePathFailures;
		if (ConsecutivePathFailures >= PatrolPoints.Num())
		{
			SetState(ESOTMIsabelAIState::Idle, TEXT("all patrol points failed pathing"));
			return;
		}
		AdvancePatrolPoint();
	}
	else if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		StartPatrolWait(Point->WaitDuration);
	}
}

void ASOTMIsabelAIController::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (CurrentState == ESOTMIsabelAIState::Chase)
	{
		// A finished chase move is information, not an excuse to idle: re-evaluate
		// against the player's CURRENT location at once. Aborted moves are ignored
		// here on purpose: they are superseded requests or attack transitions, and
		// reacting to them would re-issue moves from inside MoveToActor (recursion).
		// (Completion results are EPathFollowingResult: Success/Blocked/OffPath/
		// Aborted/Invalid — Failed belongs to the request-result enum instead.)
		// The partial/stall guards in RepathChaseIfNeeded keep this from looping.
		if (Result != EPathFollowingResult::Aborted)
		{
			if (AActor* Target = CurrentTarget.Get(); Target && bCanSeePlayer && IsValidLivingPlayer(Target))
			{
				// Within striking range the normal EvaluateChase/attack flow owns the next step.
				if (GetAttackDistance() > AttackRange)
				{
					RepathChaseIfNeeded(true);
				}
			}
		}
		return;
	}

	if (CurrentState != ESOTMIsabelAIState::Patrol || bWaitingAtPatrolPoint)
	{
		return;
	}

	if (Result == EPathFollowingResult::Success)
	{
		ConsecutivePathFailures = 0;
		if (const ASOTMIsabelPatrolPoint* Point = GetCurrentPatrolPoint())
		{
			StartPatrolWait(Point->WaitDuration);
		}
	}
	else
	{
		++ConsecutivePathFailures;
		if (ConsecutivePathFailures >= PatrolPoints.Num())
		{
			SetState(ESOTMIsabelAIState::Idle, TEXT("patrol path failures exhausted route"));
		}
		else
		{
			AdvancePatrolPoint();
		}
	}
}

void ASOTMIsabelAIController::StartPatrolWait(float Duration)
{
	bWaitingAtPatrolPoint = true;
	GetWorldTimerManager().SetTimer(PatrolWaitTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bWaitingAtPatrolPoint = false;
		AdvancePatrolPoint();
	}), FMath::Max(0.01f, Duration), false);
}

void ASOTMIsabelAIController::AdvancePatrolPoint()
{
	if (PatrolPoints.IsEmpty() || CurrentState != ESOTMIsabelAIState::Patrol)
	{
		return;
	}

	if (PatrolTraversal == ESOTMPatrolTraversalMode::BackAndForth && PatrolPoints.Num() > 1)
	{
		if (CurrentPatrolIndex + PatrolDirection >= PatrolPoints.Num() || CurrentPatrolIndex + PatrolDirection < 0)
		{
			PatrolDirection *= -1;
		}
		CurrentPatrolIndex += PatrolDirection;
	}
	else
	{
		CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
	}
	MoveToCurrentPatrolPoint();
}

void ASOTMIsabelAIController::ClearPlayerTarget(bool bSearchLastKnown)
{
	ResetJumpScareEncounter(bSearchLastKnown ? TEXT("target lost; search begins") : TEXT("target fully cleared"));
	bCanSeePlayer = false;
	CurrentTarget.Reset();
	StopMovement();
	if (bSearchLastKnown)
	{
		BeginSearchAtLastKnownLocation();
	}
}

void ASOTMIsabelAIController::BeginSearchAtLastKnownLocation()
{
	ResetJumpScareEncounter(TEXT("search state entered"));
	SetMovementSpeed(InvestigateSpeed);
	SetState(ESOTMIsabelAIState::SearchLastKnown, TEXT("sight lost"));
	SearchEndTime = GetWorld()->GetTimeSeconds() + SearchDuration;
	NextSearchMoveTime = GetWorld()->GetTimeSeconds() + 1.25f;
	const EPathFollowingRequestResult::Type Result = MoveToLocation(LastKnownPlayerLocation, AcceptanceRadius, true, true, true, false);
	bLastPathRequestValid = Result != EPathFollowingRequestResult::Failed;
}

void ASOTMIsabelAIController::BeginReturnToPatrol()
{
	ResetJumpScareEncounter(TEXT("return-to-patrol entered"));
	StopMovement();
	bCanSeePlayer = false;
	CurrentTarget.Reset();
	SetMovementSpeed(PatrolSpeed);

	if (PatrolPoints.IsEmpty())
	{
		SetState(ESOTMIsabelAIState::Idle, TEXT("no route to return to"));
		return;
	}

	const FVector PawnLocation = GetPawn()->GetActorLocation();
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < PatrolPoints.Num(); ++Index)
	{
		if (const ASOTMIsabelPatrolPoint* Point = PatrolPoints[Index].Get())
		{
			const float DistanceSquared = FVector::DistSquared(PawnLocation, Point->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				CurrentPatrolIndex = Index;
			}
		}
	}

	SetState(ESOTMIsabelAIState::ReturnToPatrol, TEXT("search/investigation complete"));
	ASOTMIsabelPatrolPoint* ReturnPoint = GetCurrentPatrolPoint();
	if (!ReturnPoint)
	{
		SetState(ESOTMIsabelAIState::Idle, TEXT("no valid patrol point remains"));
		bLastPathRequestValid = false;
		return;
	}
	const EPathFollowingRequestResult::Type Result = MoveToActor(ReturnPoint, AcceptanceRadius, true, true, true, nullptr, true);
	bLastPathRequestValid = Result != EPathFollowingRequestResult::Failed;
	if (Result == EPathFollowingRequestResult::Failed || Result == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		EnterPatrol();
	}
	else
	{
		GetWorldTimerManager().SetTimer(PatrolWaitTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (CurrentState == ESOTMIsabelAIState::ReturnToPatrol && GetMoveStatus() == EPathFollowingStatus::Idle)
			{
				GetWorldTimerManager().ClearTimer(PatrolWaitTimer);
				EnterPatrol();
			}
		}), 0.25f, true);
	}
}

void ASOTMIsabelAIController::EvaluateChase()
{
	AActor* Target = CurrentTarget.Get();
	if (!Target || !bCanSeePlayer || !IsValidLivingPlayer(Target))
	{
		return;
	}

	if (GetAttackDistance() <= AttackRange)
	{
		StopMovement();
		FaceAttackTarget();
		if (ShouldBeginJumpScare(Target))
		{
			BeginJumpScare(Target);
			return;
		}
		if (TryBeginAttack())
		{
			return;
		}
		// Hold position while the target remains in striking range but cooldown is active.
		return;
	}

	// Set movement speed and log
	SetMovementSpeed(ChaseSpeed);
	
	// Force speed in case something resets it
	if (ACharacter* Char = Cast<ACharacter>(GetPawn()))
	{
		Char->GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL MOVE SPEED: %.0f (forced)"), ChaseSpeed);
	}
	
	RepathChaseIfNeeded(false);
}

void ASOTMIsabelAIController::EvaluateAttack()
{
	AActor* Target = CurrentTarget.Get();
	if (!Target || !IsValidLivingPlayer(Target))
	{
		AbortAttack(TEXT("attack target became unavailable"));
		ClearPlayerTarget(false);
		BeginReturnToPatrol();
		return;
	}

	if (!bCanSeePlayer || !HasAttackLineOfSight())
	{
		LastKnownPlayerLocation = Target->GetActorLocation();
		AbortAttack(TEXT("attack line of sight lost"));
		ClearPlayerTarget(true);
		return;
	}

	if (GetAttackDistance() > AttackRange + AttackExitMargin)
	{
		AbortAttack(TEXT("target escaped attack range"));
		SetMovementSpeed(ChaseSpeed);
		SetState(ESOTMIsabelAIState::Chase, TEXT("target left attack range"));
		RepathChaseIfNeeded(true);
	}
}

bool ASOTMIsabelAIController::TryBeginAttack()
{
	if (!CanAttackNow())
	{
		return false;
	}
	BeginAttack();
	return true;
}

void ASOTMIsabelAIController::BeginAttack()
{
	StopMovement();
	SetMovementSpeed(0.0f);
	EnterAttackRotationMode();
	FaceAttackTarget();
	bAttackInProgress = true;
	bDamageAppliedThisAttack = false;
	SetState(ESOTMIsabelAIState::Attack, TEXT("living visible player in attack range"));

	if (AttackWindUpSeconds > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(AttackWindUpTimer, this, &ThisClass::StartAttackMontage,
			AttackWindUpSeconds, false);
	}
	else
	{
		StartAttackMontage();
	}
}

void ASOTMIsabelAIController::StartAttackMontage()
{
	if (CurrentState != ESOTMIsabelAIState::Attack || !bAttackInProgress || !IsValidLivingPlayer(CurrentTarget.Get()))
	{
		AbortAttack(TEXT("attack invalid before montage start"));
		return;
	}

	ACharacter* IsabelCharacter = Cast<ACharacter>(GetPawn());
	UAnimInstance* AnimInstance = IsabelCharacter && IsabelCharacter->GetMesh()
		? IsabelCharacter->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* Montage = NormalAttackMontage.LoadSynchronous();
	if (!AnimInstance || !Montage)
	{
		UE_LOG(LogSOTMIsabelAI, Error, TEXT("Isabel Phase 2 attack could not start: AnimInstance=%s Montage=%s"),
			*GetNameSafe(AnimInstance), *GetNameSafe(Montage));
		AbortAttack(TEXT("normal attack montage unavailable"));
		SetMovementSpeed(ChaseSpeed);
		SetState(ESOTMIsabelAIState::Chase, TEXT("attack montage unavailable"));
		return;
	}

	const float PlayedLength = AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
	if (PlayedLength <= 0.0f)
	{
		UE_LOG(LogSOTMIsabelAI, Error, TEXT("Isabel Phase 2 attack montage failed to play: %s"), *GetNameSafe(Montage));
		AbortAttack(TEXT("normal attack montage failed to play"));
		SetMovementSpeed(ChaseSpeed);
		SetState(ESOTMIsabelAIState::Chase, TEXT("attack montage failed"));
		return;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ThisClass::HandleAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	NextAttackAllowedTime = GetWorld()->GetTimeSeconds() + AttackCooldown;
	UE_LOG(LogSOTMIsabelAI, Verbose, TEXT("Isabel normal attack started; cooldown begins now (%.2fs)"), AttackCooldown);
}

void ASOTMIsabelAIController::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bAttackInProgress || CurrentState != ESOTMIsabelAIState::Attack)
	{
		return;
	}

	if (bInterrupted || AttackRecoverySeconds <= KINDA_SMALL_NUMBER)
	{
		FinishAttack();
		return;
	}

	GetWorldTimerManager().SetTimer(AttackRecoveryTimer, this, &ThisClass::FinishAttack,
		AttackRecoverySeconds, false);
}

void ASOTMIsabelAIController::FinishAttack()
{
	GetWorldTimerManager().ClearTimer(AttackWindUpTimer);
	GetWorldTimerManager().ClearTimer(AttackRecoveryTimer);
	bAttackInProgress = false;
	RestoreLocomotionRotationMode();

	AActor* Target = CurrentTarget.Get();
	if (!Target || !IsValidLivingPlayer(Target))
	{
		ClearPlayerTarget(false);
		BeginReturnToPatrol();
		return;
	}
	if (!bCanSeePlayer || !HasAttackLineOfSight())
	{
		LastKnownPlayerLocation = Target->GetActorLocation();
		ClearPlayerTarget(true);
		return;
	}

	SetMovementSpeed(ChaseSpeed);
	SetState(ESOTMIsabelAIState::Chase, TEXT("normal attack completed"));
	EvaluateChase();
}

void ASOTMIsabelAIController::AbortAttack(const TCHAR* Reason)
{
	GetWorldTimerManager().ClearTimer(AttackWindUpTimer);
	GetWorldTimerManager().ClearTimer(AttackRecoveryTimer);

	if (ACharacter* IsabelCharacter = Cast<ACharacter>(GetPawn()))
	{
		if (UAnimInstance* AnimInstance = IsabelCharacter->GetMesh() ? IsabelCharacter->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (UAnimMontage* Montage = NormalAttackMontage.Get(); Montage && AnimInstance->Montage_IsPlaying(Montage))
			{
				FOnMontageEnded EmptyEndDelegate;
				AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, Montage);
				AnimInstance->Montage_Stop(0.12f, Montage);
			}
		}
	}

	if (bAttackInProgress)
	{
		UE_LOG(LogSOTMIsabelAI, Verbose, TEXT("Isabel normal attack aborted (%s)"), Reason);
	}
	bAttackInProgress = false;
	RestoreLocomotionRotationMode();
}

void ASOTMIsabelAIController::FaceAttackTarget()
{
	APawn* IsabelPawn = GetPawn();
	const AActor* Target = CurrentTarget.Get();
	if (!IsabelPawn || !Target)
	{
		return;
	}
	const FVector ToTarget = Target->GetActorLocation() - IsabelPawn->GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		FRotator FacingRotation = ToTarget.Rotation();
		FacingRotation.Pitch = 0.0f;
		FacingRotation.Roll = 0.0f;
		IsabelPawn->SetActorRotation(FacingRotation);
	}
}

void ASOTMIsabelAIController::EnterAttackRotationMode()
{
	ACharacter* IsabelCharacter = Cast<ACharacter>(GetPawn());
	if (!IsabelCharacter || bAttackRotationModeActive)
	{
		return;
	}
	UCharacterMovementComponent* Movement = IsabelCharacter->GetCharacterMovement();
	bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
	bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
	bSavedUseControllerRotationYaw = IsabelCharacter->bUseControllerRotationYaw;
	Movement->StopMovementImmediately();
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = false;
	IsabelCharacter->bUseControllerRotationYaw = false;
	bAttackRotationModeActive = true;
}

void ASOTMIsabelAIController::RestoreLocomotionRotationMode()
{
	ACharacter* IsabelCharacter = Cast<ACharacter>(GetPawn());
	if (!IsabelCharacter || !bAttackRotationModeActive)
	{
		return;
	}
	UCharacterMovementComponent* Movement = IsabelCharacter->GetCharacterMovement();
	Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
	Movement->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
	IsabelCharacter->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	bAttackRotationModeActive = false;
}

void ASOTMIsabelAIController::HandleAttackImpactNotify()
{
	if (!bAttackInProgress || CurrentState != ESOTMIsabelAIState::Attack || bDamageAppliedThisAttack)
	{
		return;
	}

	// Consume the impact before validation so a single montage can never apply damage twice.
	bDamageAppliedThisAttack = true;
	AActor* Target = CurrentTarget.Get();
	if (!IsValidLivingPlayer(Target) || GetAttackDistance() > AttackRange || !HasAttackLineOfSight() || !IsFacingAttackTarget())
	{
		UE_LOG(LogSOTMIsabelAI, Verbose, TEXT("Isabel attack impact missed or was invalid"));
		return;
	}

	USOTMPlayerVitalComponent* TargetVitals = Target->FindComponentByClass<USOTMPlayerVitalComponent>();

	const float HealthBeforeDamage = TargetVitals ? TargetVitals->GetCurrentHealth() : 0.0f;
	const float DamagePathResult = USOTMPlayerBlueprintLibrary::ApplyPlayerDamage(this, Target, AttackDamage, this, GetPawn());
	const float HealthAfterDamage = TargetVitals ? TargetVitals->GetCurrentHealth() : 0.0f;
	const float AuthoritativeDamage = TargetVitals
		? FMath::Max(0.0f, HealthBeforeDamage - HealthAfterDamage)
		: FMath::Max(0.0f, DamagePathResult);
	if (AuthoritativeDamage > 0.0f)
	{
		LastSuccessfulDamageTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel normal attack applied %.1f damage through Player System"), AuthoritativeDamage);
	}
	else
	{
		UE_LOG(LogSOTMIsabelAI, Verbose, TEXT("Isabel normal attack was ignored by Player System (invulnerable/unavailable)"));
	}

	if (!IsValidLivingPlayer(Target))
	{
		BeginIsabelDeathTransition();
		AbortAttack(TEXT("player died from attack"));
		ClearPlayerTarget(false);
		BeginReturnToPatrol();
	}
}

bool ASOTMIsabelAIController::ShouldBeginJumpScare(const AActor* Target) const
{
	if (!bEnableJumpScare || CurrentState != ESOTMIsabelAIState::Chase || bJumpScareInProgress ||
		bJumpScarePlayedThisEncounter || Target != CurrentTarget.Get() || !IsValidLivingPlayer(Target) ||
		IsPlayerDeadOrRespawning())
	{
		return false;
	}

	return GetAttackDistance() <= AttackRange && HasAttackLineOfSight() && IsFacingAttackTarget();
}

void ASOTMIsabelAIController::BeginJumpScare(AActor* Target)
{
	if (!ShouldBeginJumpScare(Target))
	{
		return;
	}

	AbortAttack(TEXT("catch jump scare started"));
	StopMovement();
	SetMovementSpeed(0.0f);
	EnterAttackRotationMode();
	CurrentTarget = Target;
	JumpScareTarget = Target;
	FaceAttackTarget();

	bJumpScareInProgress = true;
	bJumpScarePlayedThisEncounter = true;
	bJumpScareImpactReached = false;
	bJumpScareMontageStarted = false;
	bJumpScareSoundPlayed = false;
	JumpScareImpactStartTime = -1.0f;
	JumpScareCinematicStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::Catch;
	SetState(ESOTMIsabelAIState::JumpScare, TEXT("first valid catch in chase encounter"));

	USOTMPlayerBlueprintLibrary::AcquirePlayerInputLock(this, ESOTMInputLockReason::JumpScare);
	bJumpScareInputLockHeld = true;
	const bool bCameraReady = CreateJumpScareCamera(Target);
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel catch jump scare started: target=%s camera=%s lock=held damage=none"),
		*GetNameSafe(Target), bCameraReady ? TEXT("ready") : TEXT("unavailable"));

	if (JumpScareCameraBlendInSeconds > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(JumpScareStartTimer, this, &ThisClass::StartJumpScareMontage,
			JumpScareCameraBlendInSeconds, false);
	}
	else
	{
		StartJumpScareMontage();
	}
}

void ASOTMIsabelAIController::StartJumpScareMontage()
{
	if (!bJumpScareInProgress || CurrentState != ESOTMIsabelAIState::JumpScare || !IsValidLivingPlayer(JumpScareTarget.Get()))
	{
		AbortJumpScare(TEXT("jump-scare target became unavailable before montage"), false);
		return;
	}

	ACharacter* IsabelCharacter = Cast<ACharacter>(GetPawn());
	UAnimInstance* AnimInstance = IsabelCharacter && IsabelCharacter->GetMesh()
		? IsabelCharacter->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* Montage = JumpScareMontage.LoadSynchronous();
	if (!AnimInstance || !Montage)
	{
		UE_LOG(LogSOTMIsabelAI, Error, TEXT("Isabel Phase 3 montage unavailable: AnimInstance=%s Montage=%s"),
			*GetNameSafe(AnimInstance), *GetNameSafe(Montage));
		AbortJumpScare(TEXT("jump-scare montage unavailable"), false);
		if (IsValidLivingPlayer(CurrentTarget.Get()))
		{
			SetMovementSpeed(ChaseSpeed);
			SetState(ESOTMIsabelAIState::Chase, TEXT("jump-scare montage unavailable"));
		}
		return;
	}

	const float PlayedLength = AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
	if (PlayedLength <= 0.0f)
	{
		UE_LOG(LogSOTMIsabelAI, Error, TEXT("Isabel Phase 3 montage failed to play: %s"), *GetNameSafe(Montage));
		AbortJumpScare(TEXT("jump-scare montage failed"), false);
		return;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ThisClass::HandleJumpScareMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	bJumpScareMontageStarted = true;
	JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::Anticipation;
	JumpScareCinematicStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// Audio is deliberately deferred to the dedicated impact notify so the
	// strongest visual motion, sound and camera impulse share one sync point.
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel cinematic animation started (%.2fs montage; audio waits for impact)"), PlayedLength);
}

void ASOTMIsabelAIController::HandleJumpScareImpactNotify()
{
	if (!bJumpScareInProgress || CurrentState != ESOTMIsabelAIState::JumpScare || bJumpScareImpactReached)
	{
		return;
	}

	// The catch cinematic is presentation-only. This notify remains the exact
	// synchronization point for camera/audio effects, but never applies damage.
	bJumpScareImpactReached = true;
	JumpScareImpactStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::Impact;

	if (!bJumpScareSoundPlayed)
	{
		bJumpScareSoundPlayed = true;
		if (USoundBase* Sound = JumpScareSound.LoadSynchronous())
		{
			JumpScareAudioComponent = UGameplayStatics::SpawnSound2D(
				this, Sound, 1.0f, 1.0f, 0.0f, nullptr, false, true);
		}
		else
		{
			UE_LOG(LogSOTMIsabelAI, Warning, TEXT("Isabel cinematic scream audio is unavailable"));
		}
	}
	UpdateJumpScareCameraTracking();
	UE_LOG(LogSOTMIsabelAI, Display,
		TEXT("Isabel cinematic impact synchronized: scream, FOV, post process and camera impulse; damage=none"));
}

void ASOTMIsabelAIController::HandleJumpScareMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bJumpScareInProgress || CurrentState != ESOTMIsabelAIState::JumpScare)
	{
		return;
	}

	if (!bInterrupted && !bJumpScareImpactReached)
	{
		UE_LOG(LogSOTMIsabelAI, Warning,
			TEXT("Isabel catch jump-scare montage completed without its presentation impact notify"));
	}
	FinishJumpScare();
}

void ASOTMIsabelAIController::FinishJumpScare()
{
	GetWorldTimerManager().ClearTimer(JumpScareStartTimer);
	GetWorldTimerManager().ClearTimer(JumpScareResumeTimer);
	bJumpScareInProgress = false;
	bJumpScareMontageStarted = false;
	JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::Recovery;
	ClearJumpScareCinematicEffects();
	if (UAudioComponent* Audio = JumpScareAudioComponent.Get())
	{
		Audio->FadeOut(0.12f, 0.0f);
	}
	JumpScareAudioComponent.Reset();
	RestorePlayerCamera(false);
	RestoreLocomotionRotationMode();

	AActor* Target = JumpScareTarget.Get();
	JumpScareTarget.Reset();
	if (!Target || !IsValidLivingPlayer(Target))
	{
		ClearPlayerTarget(false);
		BeginReturnToPatrol();
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel catch jump scare completed without a valid living target"));
		return;
	}

	CurrentTarget = Target;
	const float ResumeDelay = FMath::Max(0.0f, JumpScareCameraBlendOutSeconds) + 0.02f;
	GetWorldTimerManager().SetTimer(JumpScareResumeTimer, this, &ThisClass::ResumeAfterJumpScare,
		ResumeDelay, false);
	UE_LOG(LogSOTMIsabelAI, Display,
		TEXT("Isabel catch jump scare completed; waiting %.2fs for camera/input restoration"), ResumeDelay);
}

void ASOTMIsabelAIController::ResumeAfterJumpScare()
{
	AActor* Target = CurrentTarget.Get();
	if (!Target || !IsValidLivingPlayer(Target))
	{
		ClearPlayerTarget(false);
		BeginReturnToPatrol();
		return;
	}

	if (!bCanSeePlayer || !HasAttackLineOfSight())
	{
		LastKnownPlayerLocation = Target->GetActorLocation();
		ClearPlayerTarget(true);
		return;
	}

	SetMovementSpeed(ChaseSpeed);
	SetState(ESOTMIsabelAIState::Chase, TEXT("catch cinematic camera and input restored"));
	EvaluateChase();
}

void ASOTMIsabelAIController::AbortJumpScare(const TCHAR* Reason, const bool bRestoreCameraImmediately)
{
	GetWorldTimerManager().ClearTimer(JumpScareStartTimer);
	GetWorldTimerManager().ClearTimer(JumpScareResumeTimer);
	if (!bJumpScareInProgress && !bJumpScareInputLockHeld && !JumpScareCamera.IsValid())
	{
		return;
	}

	bJumpScareInProgress = false;
	bJumpScareMontageStarted = false;
	JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::None;
	ClearJumpScareCinematicEffects();
	if (UAudioComponent* Audio = JumpScareAudioComponent.Get())
	{
		Audio->Stop();
	}
	JumpScareAudioComponent.Reset();
	if (ACharacter* IsabelCharacter = Cast<ACharacter>(GetPawn()))
	{
		if (UAnimInstance* AnimInstance = IsabelCharacter->GetMesh() ? IsabelCharacter->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (UAnimMontage* Montage = JumpScareMontage.Get(); Montage && AnimInstance->Montage_IsPlaying(Montage))
			{
				FOnMontageEnded EmptyEndDelegate;
				AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, Montage);
				AnimInstance->Montage_Stop(0.08f, Montage);
			}
		}
	}

	RestorePlayerCamera(bRestoreCameraImmediately);
	RestoreLocomotionRotationMode();
	ResetJumpScareEncounter(TEXT("jump scare aborted before completion"));
	JumpScareTarget.Reset();
	UE_LOG(LogSOTMIsabelAI, Verbose, TEXT("Isabel Phase 3 jump scare aborted (%s)"), Reason);
}

void ASOTMIsabelAIController::ResetJumpScareEncounter(const TCHAR* Reason)
{
	if (bJumpScarePlayedThisEncounter || bJumpScareImpactReached)
	{
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel catch encounter reset (%s)"), Reason);
	}
	bJumpScarePlayedThisEncounter = false;
	bJumpScareImpactReached = false;
}

bool ASOTMIsabelAIController::CreateJumpScareCamera(AActor* Target)
{
	UWorld* World = GetWorld();
	APawn* IsabelPawn = GetPawn();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !IsabelPawn || !Target || !PlayerController)
	{
		return false;
	}

	const FVector IsabelLocation = IsabelPawn->GetActorLocation();
	const ACharacter* IsabelCharacter = Cast<ACharacter>(IsabelPawn);
	const USkeletalMeshComponent* IsabelMesh = IsabelCharacter ? IsabelCharacter->GetMesh() : nullptr;
	const FVector FocusLocation = IsabelMesh && IsabelMesh->DoesSocketExist(JumpScareFocusBone)
		? IsabelMesh->GetSocketLocation(JumpScareFocusBone)
		: IsabelLocation + FVector(0.0f, 0.0f, JumpScareFocusHeight);
	FVector TowardPlayer = Target->GetActorLocation() - IsabelLocation;
	TowardPlayer.Z = 0.0f;
	TowardPlayer = TowardPlayer.GetSafeNormal();
	if (TowardPlayer.IsNearlyZero())
	{
		TowardPlayer = IsabelPawn->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, TowardPlayer).GetSafeNormal();
	FVector DesiredLocation = FocusLocation + TowardPlayer * JumpScareCameraDistance + Right * JumpScareCameraSideOffset;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(IsabelJumpScareCamera), false, IsabelPawn);
	TraceParams.AddIgnoredActor(Target);
	FHitResult CameraHit;
	if (World->LineTraceSingleByChannel(CameraHit, FocusLocation, DesiredLocation, ECC_Visibility, TraceParams))
	{
		const FVector TraceDirection = (DesiredLocation - FocusLocation).GetSafeNormal();
		DesiredLocation = CameraHit.ImpactPoint - TraceDirection * 12.0f;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = IsabelPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;
	ACameraActor* Camera = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(), DesiredLocation, (FocusLocation - DesiredLocation).Rotation(), SpawnParameters);
	if (!Camera)
	{
		return false;
	}
	if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(JumpScareAnticipationFOV);
		CameraComponent->SetConstraintAspectRatio(false);
	}

	JumpScareCamera = Camera;
	JumpScareCameraBaseLocation = DesiredLocation;
	JumpScareCinematicStartTime = World->GetTimeSeconds();
	PlayerController->SetViewTargetWithBlend(
		Camera, JumpScareCameraBlendInSeconds, EViewTargetBlendFunction::VTBlend_Cubic, 2.0f, true);
	World->GetTimerManager().SetTimer(
		JumpScareCameraTrackingTimer, this, &ThisClass::UpdateJumpScareCameraTracking, 1.0f / 30.0f, true);
	UpdateJumpScareCameraTracking();
	return true;
}

void ASOTMIsabelAIController::UpdateJumpScareCameraTracking()
{
	ACameraActor* Camera = JumpScareCamera.Get();
	ACharacter* IsabelCharacter = Cast<ACharacter>(GetPawn());
	const USkeletalMeshComponent* IsabelMesh = IsabelCharacter ? IsabelCharacter->GetMesh() : nullptr;
	UWorld* World = GetWorld();
	if (!Camera || !IsabelCharacter || !IsabelMesh || !World)
	{
		return;
	}

	const FVector FocusLocation = IsabelMesh->DoesSocketExist(JumpScareFocusBone)
		? IsabelMesh->GetSocketLocation(JumpScareFocusBone)
		: IsabelCharacter->GetActorLocation() + FVector(0.0f, 0.0f, JumpScareFocusHeight);
	const float Now = World->GetTimeSeconds();
	const float DeltaSeconds = 1.0f / 30.0f;
	float EffectStrength = 0.0f;
	float TargetFOV = JumpScareCameraFOV;
	float PushInAlpha = 0.0f;
	float ImpactDecay = 0.0f;

	switch (JumpScareCinematicPhase)
	{
	case ESOTMJumpScareCinematicPhase::Catch:
	{
		const float CatchAlpha = FMath::Clamp(
			(Now - JumpScareCinematicStartTime) / FMath::Max(0.05f, JumpScareCameraBlendInSeconds), 0.0f, 1.0f);
		TargetFOV = FMath::Lerp(JumpScareAnticipationFOV, JumpScareCameraFOV, CatchAlpha);
		EffectStrength = 0.08f * CatchAlpha;
		break;
	}
	case ESOTMJumpScareCinematicPhase::Anticipation:
	{
		const float AnticipationAlpha = FMath::Clamp((Now - JumpScareCinematicStartTime) / 0.4f, 0.0f, 1.0f);
		PushInAlpha = 0.45f * AnticipationAlpha;
		TargetFOV = FMath::Lerp(JumpScareCameraFOV, JumpScareImpactFOV, 0.12f * AnticipationAlpha);
		EffectStrength = 0.12f + 0.08f * AnticipationAlpha;
		break;
	}
	case ESOTMJumpScareCinematicPhase::Impact:
	{
		const float ImpactAge = FMath::Max(0.0f, Now - JumpScareImpactStartTime);
		ImpactDecay = 1.0f - FMath::Clamp(ImpactAge / FMath::Max(0.05f, JumpScareImpactEffectSeconds), 0.0f, 1.0f);
		PushInAlpha = 0.45f + 0.55f * ImpactDecay;
		TargetFOV = FMath::Lerp(JumpScareCameraFOV, JumpScareImpactFOV, ImpactDecay);
		EffectStrength = FMath::Lerp(0.12f, 1.0f, ImpactDecay);
		if (ImpactDecay <= KINDA_SMALL_NUMBER)
		{
			JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::Recovery;
		}
		break;
	}
	case ESOTMJumpScareCinematicPhase::Recovery:
		PushInAlpha = 0.25f;
		TargetFOV = JumpScareCameraFOV;
		EffectStrength = 0.08f;
		break;
	default:
		break;
	}

	const FVector TowardFocus = (FocusLocation - JumpScareCameraBaseLocation).GetSafeNormal();
	const FVector CameraRight = FVector::CrossProduct(FVector::UpVector, TowardFocus).GetSafeNormal();
	FVector DesiredLocation = JumpScareCameraBaseLocation + TowardFocus * (JumpScareCameraPushInDistance * PushInAlpha);
	FRotator ImpactRotationOffset = FRotator::ZeroRotator;
	if (ImpactDecay > 0.0f)
	{
		const float ImpactAge = Now - JumpScareImpactStartTime;
		DesiredLocation += CameraRight * FMath::Sin(ImpactAge * 68.0f) * JumpScareImpactLocationAmplitude * ImpactDecay;
		DesiredLocation += FVector::UpVector * FMath::Cos(ImpactAge * 79.0f) * JumpScareImpactLocationAmplitude * 0.55f * ImpactDecay;
		ImpactRotationOffset.Pitch = FMath::Sin(ImpactAge * 73.0f) * JumpScareImpactRotationAmplitude * ImpactDecay;
		ImpactRotationOffset.Yaw = FMath::Cos(ImpactAge * 61.0f) * JumpScareImpactRotationAmplitude * 0.7f * ImpactDecay;
	}

	Camera->SetActorLocation(DesiredLocation);
	const FRotator DesiredRotation = (FocusLocation - DesiredLocation).Rotation() + ImpactRotationOffset;
	Camera->SetActorRotation(FMath::RInterpTo(
		Camera->GetActorRotation(), DesiredRotation, DeltaSeconds, JumpScareCameraTrackingSpeed));
	ApplyJumpScareCinematicEffects(EffectStrength, TargetFOV);

	// Use the blend-in as a short, pop-free facing window. Once the montage starts,
	// animation owns Isabel's pose and rotation is no longer adjusted.
	if (!bJumpScareMontageStarted)
	{
		FVector ToCamera = DesiredLocation - IsabelCharacter->GetActorLocation();
		ToCamera.Z = 0.0f;
		if (!ToCamera.IsNearlyZero())
		{
			const FRotator FacingRotation(0.0f, ToCamera.Rotation().Yaw, 0.0f);
			IsabelCharacter->SetActorRotation(FMath::RInterpTo(
				IsabelCharacter->GetActorRotation(), FacingRotation, DeltaSeconds, JumpScareCameraTrackingSpeed));
		}
	}
}

void ASOTMIsabelAIController::ApplyJumpScareCinematicEffects(const float EffectStrength, const float TargetFOV)
{
	ACameraActor* Camera = JumpScareCamera.Get();
	UCameraComponent* CameraComponent = Camera ? Camera->GetCameraComponent() : nullptr;
	if (!CameraComponent)
	{
		return;
	}

	const float Strength = FMath::Clamp(EffectStrength, 0.0f, 1.0f);
	CameraComponent->SetFieldOfView(TargetFOV);
	CameraComponent->SetPostProcessBlendWeight(1.0f);
	FPostProcessSettings& Settings = CameraComponent->PostProcessSettings;
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = FMath::Lerp(0.18f, JumpScarePeakVignette, Strength);
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = JumpScarePeakExposureBias * Strength;
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = JumpScarePeakChromaticAberration * Strength;
}

void ASOTMIsabelAIController::ClearJumpScareCinematicEffects()
{
	ACameraActor* Camera = JumpScareCamera.Get();
	UCameraComponent* CameraComponent = Camera ? Camera->GetCameraComponent() : nullptr;
	if (CameraComponent)
	{
		CameraComponent->SetFieldOfView(JumpScareCameraFOV);
		CameraComponent->SetPostProcessBlendWeight(0.0f);
		FPostProcessSettings& Settings = CameraComponent->PostProcessSettings;
		Settings.bOverride_VignetteIntensity = false;
		Settings.bOverride_AutoExposureBias = false;
		Settings.bOverride_SceneFringeIntensity = false;
	}
	JumpScareCinematicPhase = ESOTMJumpScareCinematicPhase::None;
	JumpScareImpactStartTime = -1.0f;
	JumpScareCameraBaseLocation = FVector::ZeroVector;
}

void ASOTMIsabelAIController::BeginIsabelDeathTransition()
{
	if (bIsabelDeathFadeActive)
	{
		return;
	}

	bIsabelDeathFadeActive = true;
	if (IsabelDeathAnimationLeadInSeconds <= KINDA_SMALL_NUMBER)
	{
		StartIsabelDeathFade();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			IsabelDeathFadeTimer,
			this,
			&ASOTMIsabelAIController::StartIsabelDeathFade,
			IsabelDeathAnimationLeadInSeconds,
			false);
	}

	UE_LOG(LogSOTMIsabelAI, Display,
		TEXT("Isabel lethal normal attack started Player System death presentation; fade begins after %.2fs"),
		IsabelDeathAnimationLeadInSeconds);
}

void ASOTMIsabelAIController::StartIsabelDeathFade()
{
	if (!bIsabelDeathFadeActive)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager : nullptr;
	if (!CameraManager)
	{
		return;
	}

	CameraManager->StartCameraFade(
		0.0f, IsabelDeathFadeOpacity, IsabelDeathFadeSeconds, FLinearColor::Black, false, true);
	UE_LOG(LogSOTMIsabelAI, Display,
		TEXT("Isabel controlled death fade started after player death animation lead-in."));
}

void ASOTMIsabelAIController::ClearIsabelDeathTransition(const bool bFadeBackToGameplay)
{
	GetWorldTimerManager().ClearTimer(IsabelDeathFadeTimer);
	if (!bIsabelDeathFadeActive)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager : nullptr;
	if (CameraManager)
	{
		if (bFadeBackToGameplay)
		{
			CameraManager->StartCameraFade(
				IsabelDeathFadeOpacity, 0.0f, 0.4f, FLinearColor::Black, false, false);
		}
		else
		{
			CameraManager->StopCameraFade();
		}
	}
	bIsabelDeathFadeActive = false;
}

void ASOTMIsabelAIController::RestorePlayerCamera(const bool bImmediate)
{
	GetWorldTimerManager().ClearTimer(JumpScareCameraReleaseTimer);
	GetWorldTimerManager().ClearTimer(JumpScareCameraTrackingTimer);
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	ACameraActor* Camera = JumpScareCamera.Get();
	if (!PlayerController || !PlayerPawn || !Camera)
	{
		if (Camera)
		{
			Camera->Destroy();
		}
		JumpScareCamera.Reset();
		ReleaseJumpScareInputLock();
		return;
	}

	const float BlendTime = bImmediate ? 0.0f : JumpScareCameraBlendOutSeconds;
	PlayerController->SetViewTargetWithBlend(
		PlayerPawn, BlendTime, EViewTargetBlendFunction::VTBlend_Cubic, 2.0f, true);
	if (BlendTime <= KINDA_SMALL_NUMBER)
	{
		Camera->Destroy();
		JumpScareCamera.Reset();
		ReleaseJumpScareInputLock();
		return;
	}

	GetWorldTimerManager().SetTimer(JumpScareCameraReleaseTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (ACameraActor* CameraToDestroy = JumpScareCamera.Get())
			{
				CameraToDestroy->Destroy();
			}
			JumpScareCamera.Reset();
			ReleaseJumpScareInputLock();
		}), BlendTime, false);
}

void ASOTMIsabelAIController::ReleaseJumpScareInputLock()
{
	if (!bJumpScareInputLockHeld)
	{
		return;
	}
	USOTMPlayerBlueprintLibrary::ReleasePlayerInputLock(this, ESOTMInputLockReason::JumpScare);
	bJumpScareInputLockHeld = false;
}

void ASOTMIsabelAIController::HandlePlayerRespawned(AActor* PlayerActor)
{
	ClearIsabelDeathTransition(true);
	if (bJumpScareInProgress)
	{
		AbortJumpScare(TEXT("player respawned"), true);
	}
	ResetJumpScareEncounter(TEXT("player respawned"));
	bPlayerStimulusRegistered = false;
	CurrentTarget.Reset();
	JumpScareTarget.Reset();
	bCanSeePlayer = false;
	if (IsabelPerception && PlayerActor)
	{
		IsabelPerception->ForgetActor(PlayerActor);
		IsabelPerception->RequestStimuliListenerUpdate();
	}
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("Isabel Phase 3 reset after Player System respawn"));
}
void ASOTMIsabelAIController::RepathChaseIfNeeded(bool bForce)
{
	AActor* Target = CurrentTarget.Get();
	if (!Target || !bCanSeePlayer || !IsValidLivingPlayer(Target))
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const FVector TargetLocation = Target->GetActorLocation();
	float DistToPlayer = FVector::Dist(GetPawn()->GetActorLocation(), TargetLocation);
	
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL DISTANCE TO PLAYER: %.0f"), DistToPlayer);

	// --- Chase progress accounting (stagnation guard). ---
	// A relocating player is new information every time and never counts as a stall,
	// so dynamic pursuit is never punished. Only a static player with no distance
	// closed advances the stall counter.
	const float PlayerMovedSinceRequest = FVector::Dist(TargetLocation, LastChaseRequestLocation);
	if (SOTMIsabelChaseFix::LastProgressDistance < 0.0f)
	{
		// No baseline yet (fresh chase): record it without judging.
		SOTMIsabelChaseFix::LastProgressDistance = DistToPlayer;
		SOTMIsabelChaseFix::StallCount = 0;
	}
	else if (PlayerMovedSinceRequest >= ChaseRepathDistance)
	{
		SOTMIsabelChaseFix::StallCount = 0;
		SOTMIsabelChaseFix::bBlockerReported = false;
	}
	else if (SOTMIsabelChaseFix::LastProgressDistance - DistToPlayer >= SOTMIsabelChaseFix::ProgressEpsilon)
	{
		// Meaningful progress toward a static player: reset the guard.
		SOTMIsabelChaseFix::LastProgressDistance = DistToPlayer;
		SOTMIsabelChaseFix::StallCount = 0;
		SOTMIsabelChaseFix::bBlockerReported = false;
	}
	else
	{
		++SOTMIsabelChaseFix::StallCount;
	}

	// --- Stand-down: never hammer an identical doomed request. ---
	// bForce bypasses only the time cadence below, never a confirmed partial/stalled
	// block: with an unmoved player there is nothing new to request. The chase stays
	// active with CurrentTarget kept, and resumes automatically once the player moves.
	const bool bPlayerRelocated = PlayerMovedSinceRequest >= ChaseRepathDistance;
	const bool bBlocked = (SOTMIsabelChaseFix::bPartialPathActive
		|| SOTMIsabelChaseFix::StallCount >= SOTMIsabelChaseFix::MaxStallRepaths) && !bPlayerRelocated;
	if (bBlocked)
	{
		ReportIsabelChaseBlocked(this, Target, DistToPlayer, SOTMIsabelChaseFix::bPartialPathActive);
		return;
	}

	if (!bForce && Now < NextChaseRepathTime && !bPlayerRelocated)
	{
		return;
	}

	LastKnownPlayerLocation = TargetLocation;
	LastChaseRequestLocation = TargetLocation;
	NextChaseRepathTime = Now + ChaseRepathInterval;
	SOTMIsabelChaseFix::LastProgressDistance = DistToPlayer;
	
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL MOVE REQUEST: Attempting"));
	
	const EPathFollowingRequestResult::Type Result = MoveToActor(Target, AcceptanceRadius, true, true, true, nullptr, true);
	bLastPathRequestValid = Result != EPathFollowingRequestResult::Failed;
	
	UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL MOVE RESULT: %d"), (int32)Result);

	// A partial path is NOT a successful chase: record it so the next evaluation
	// stands down instead of repeating the same partial request forever.
	SOTMIsabelChaseFix::bPartialPathActive = false;
	if (Result == EPathFollowingRequestResult::RequestSuccessful)
	{
		if (const UPathFollowingComponent* PathComp = GetPathFollowingComponent())
		{
			if (const FNavPathSharedPtr ActivePath = PathComp->GetPath(); ActivePath.IsValid())
			{
				SOTMIsabelChaseFix::bPartialPathActive = ActivePath->IsPartial();
			}
		}
		if (SOTMIsabelChaseFix::bPartialPathActive)
		{
			ReportIsabelChaseBlocked(this, Target, DistToPlayer, true);
		}
	}
	
	// Log current movement status
	if (const ACharacter* Char = Cast<ACharacter>(GetPawn()))
	{
		float CurrentSpeed = Char->GetCharacterMovement()->MaxWalkSpeed;
		UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL MOVE SPEED: %.0f"), CurrentSpeed);
	}
}

void ASOTMIsabelAIController::SetMovementSpeed(float Speed) const
{
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
	{
		UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement();
		if (Movement)
		{
			Movement->MaxWalkSpeed = Speed;
			Movement->MinAnalogWalkSpeed = 0.0f;
			// Ensure movement is enabled
			Movement->SetMovementMode(MOVE_Walking);
			UE_LOG(LogSOTMIsabelAI, Display, TEXT("ISABEL MOVE SPEED: %.0f (SetMovementSpeed)"), Speed);
		}
	}
}

bool ASOTMIsabelAIController::IsValidLivingPlayer(const AActor* Actor) const
{
	const APawn* CandidatePawn = Cast<APawn>(Actor);
	const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!CandidatePawn || !PlayerController || !CandidatePawn->IsPlayerControlled() || CandidatePawn != PlayerController->GetPawn())
	{
		return false;
	}
	if (const USOTMPlayerVitalComponent* Vitals = CandidatePawn->FindComponentByClass<USOTMPlayerVitalComponent>(); Vitals && Vitals->IsDead())
	{
		return false;
	}
	if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (const USOTMPlayerStateSubsystem* PlayerFoundationState = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>();
			PlayerFoundationState && (PlayerFoundationState->IsPlayerDead() || PlayerFoundationState->IsGameOver()))
		{
			return false;
		}
	}
	return true;
}

bool ASOTMIsabelAIController::IsSightStimulus(const FAIStimulus& Stimulus) const
{
	return Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
}

bool ASOTMIsabelAIController::IsHearingStimulus(const FAIStimulus& Stimulus) const
{
	return Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>();
}

ASOTMIsabelPatrolPoint* ASOTMIsabelAIController::GetCurrentPatrolPoint() const
{
	return PatrolPoints.IsValidIndex(CurrentPatrolIndex) ? PatrolPoints[CurrentPatrolIndex].Get() : nullptr;
}

float ASOTMIsabelAIController::GetSearchTimeRemaining() const
{
	return GetWorld() ? FMath::Max(0.0f, SearchEndTime - GetWorld()->GetTimeSeconds()) : 0.0f;
}

float ASOTMIsabelAIController::GetCurrentMovementSpeed() const
{
	if (const ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
	{
		return ControlledCharacter->GetVelocity().Size2D();
	}
	return 0.0f;
}

float ASOTMIsabelAIController::GetAttackDistance() const
{
	return GetPawn() && CurrentTarget.IsValid()
		? FVector::Dist2D(GetPawn()->GetActorLocation(), CurrentTarget->GetActorLocation())
		: TNumericLimits<float>::Max();
}

bool ASOTMIsabelAIController::HasAttackLineOfSight() const
{
	const AActor* Target = CurrentTarget.Get();
	return Target && bCanSeePlayer && LineOfSightTo(Target);
}

bool ASOTMIsabelAIController::IsFacingAttackTarget() const
{
	const APawn* IsabelPawn = GetPawn();
	const AActor* Target = CurrentTarget.Get();
	if (!IsabelPawn || !Target)
	{
		return false;
	}
	const FVector ToTarget = (Target->GetActorLocation() - IsabelPawn->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return true;
	}
	const float FacingDot = FVector::DotProduct(IsabelPawn->GetActorForwardVector().GetSafeNormal2D(), ToTarget);
	const float RequiredDot = FMath::Cos(FMath::DegreesToRadians(FacingToleranceDegrees));
	return FacingDot >= RequiredDot;
}

bool ASOTMIsabelAIController::CanAttackNow() const
{
	return !bAttackInProgress && !bJumpScareInProgress && IsValidLivingPlayer(CurrentTarget.Get()) &&
		GetAttackDistance() <= AttackRange && HasAttackLineOfSight() && IsFacingAttackTarget() &&
		GetAttackCooldownRemaining() <= 0.0f;
}

float ASOTMIsabelAIController::GetAttackCooldownRemaining() const
{
	return GetWorld() ? FMath::Max(0.0f, NextAttackAllowedTime - GetWorld()->GetTimeSeconds()) : 0.0f;
}

bool ASOTMIsabelAIController::IsPlayerDeadOrRespawning() const
{
	if (!GetWorld())
	{
		return false;
	}
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (const USOTMPlayerVitalComponent* Vitals = PlayerPawn ? PlayerPawn->FindComponentByClass<USOTMPlayerVitalComponent>() : nullptr;
		Vitals && Vitals->IsDead())
	{
		return true;
	}
	if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (const USOTMPlayerStateSubsystem* State = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>())
		{
			return State->IsPlayerDead() || State->IsGameOver() ||
				State->GetInputLockCount(ESOTMInputLockReason::Respawn) > 0;
		}
	}
	return false;
}

FString ASOTMIsabelAIController::BuildDebugText() const
{
	return FString::Printf(TEXT("ISABEL [%s] target=%s seen=%s patrol=%s path=%s speed=%.0f LKP=%s search=%.1fs\n")
		TEXT("ATTACK dist=%.0f range=%.0f LOS=%s facing=%s can=%s active=%s damageConsumed=%s cooldown=%.2fs lastDamage=%.2f playerUnavailable=%s\n")
		TEXT("JUMP SCARE catchBased=yes active=%s usedThisEncounter=%s impactReached=%s camera=%s inputLock=%s"),
		StateToString(CurrentState), *GetNameSafe(CurrentTarget.Get()), bCanSeePlayer ? TEXT("yes") : TEXT("no"),
		*GetNameSafe(GetCurrentPatrolPoint()), bLastPathRequestValid ? TEXT("valid") : TEXT("invalid"), GetCurrentMovementSpeed(),
		*LastKnownPlayerLocation.ToCompactString(), GetSearchTimeRemaining(), GetAttackDistance(), AttackRange,
		HasAttackLineOfSight() ? TEXT("yes") : TEXT("no"), IsFacingAttackTarget() ? TEXT("yes") : TEXT("no"),
		CanAttackNow() ? TEXT("yes") : TEXT("no"), bAttackInProgress ? TEXT("yes") : TEXT("no"),
		bDamageAppliedThisAttack ? TEXT("yes") : TEXT("no"), GetAttackCooldownRemaining(), LastSuccessfulDamageTime,
		IsPlayerDeadOrRespawning() ? TEXT("yes") : TEXT("no"),
		bJumpScareInProgress ? TEXT("yes") : TEXT("no"), bJumpScarePlayedThisEncounter ? TEXT("yes") : TEXT("no"),
		bJumpScareImpactReached ? TEXT("yes") : TEXT("no"), JumpScareCamera.IsValid() ? TEXT("yes") : TEXT("no"),
		bJumpScareInputLockHeld ? TEXT("held") : TEXT("clear"));
}

void ASOTMIsabelAIController::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
#if !UE_BUILD_SHIPPING
	if (Canvas && GEngine)
	{
		Canvas->SetDrawColor(FColor::Purple);
		Canvas->DrawText(GEngine->GetSmallFont(), BuildDebugText(), 4.0f, YPos);
		YPos += YL;
	}
#endif
}

const TCHAR* ASOTMIsabelAIController::StateToString(ESOTMIsabelAIState State)
{
	switch (State)
	{
	case ESOTMIsabelAIState::Idle: return TEXT("Idle");
	case ESOTMIsabelAIState::Patrol: return TEXT("Patrol");
	case ESOTMIsabelAIState::Investigate: return TEXT("Investigate");
	case ESOTMIsabelAIState::Chase: return TEXT("Chase");
	case ESOTMIsabelAIState::Attack: return TEXT("Attack");
	case ESOTMIsabelAIState::JumpScare: return TEXT("JumpScare");
	case ESOTMIsabelAIState::SearchLastKnown: return TEXT("SearchLastKnown");
	case ESOTMIsabelAIState::ReturnToPatrol: return TEXT("ReturnToPatrol");
	default: return TEXT("Unknown");
	}
}
