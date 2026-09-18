#include "AI/SOTMCousinAIController.h"

#include "AI/SOTMCousinCharacter.h"
#include "Demo/SOTMDemoPhase2WorldSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMCousinAI, Log, All);

ASOTMCousinAIController::ASOTMCousinAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	CousinPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("CousinPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("CousinSight"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(LostTargetGraceSeconds);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	CousinPerception->ConfigureSense(*SightConfig);
	CousinPerception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*CousinPerception);
}

void ASOTMCousinAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	HomeLocation = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
	CousinPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	CousinPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	ReceiveMoveCompleted.RemoveDynamic(this, &ThisClass::HandleMoveCompleted);
	ReceiveMoveCompleted.AddDynamic(this, &ThisClass::HandleMoveCompleted);
	GetWorldTimerManager().SetTimer(EvaluationTimer, this, &ThisClass::EvaluateBehavior, 0.20f, true, 0.15f);
	BeginPatrol();
}

void ASOTMCousinAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (CousinPerception)
	{
		CousinPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}
	ReceiveMoveCompleted.RemoveDynamic(this, &ThisClass::HandleMoveCompleted);
	Super::OnUnPossess();
}

void ASOTMCousinAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

void ASOTMCousinAIController::SetPresentationVariant(const int32 VariantIndex)
{
	PresentationVariant = FMath::Abs(VariantIndex) % 3;
}

void ASOTMCousinAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!IsValidLivingPlayer(Actor) || CurrentState == ESOTMCousinAIState::Catch ||
		CurrentState == ESOTMCousinAIState::Disabled)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		const bool bNewEncounter = CurrentTarget.Get() != Actor || !bCanSeeTarget;
		CurrentTarget = Actor;
		bCanSeeTarget = true;
		LastKnownTargetLocation = Actor->GetActorLocation();
		LastSeenTargetTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		SetState(ESOTMCousinAIState::Chase, TEXT("player detected"));
		if (bNewEncounter)
		{
			if (USOTMDemoPhase2WorldSubsystem* Phase2 = GetWorld()->GetSubsystem<USOTMDemoPhase2WorldSubsystem>())
			{
				Phase2->NotifyCousinDetected(this);
			}
		}
	}
	else if (CurrentTarget.Get() == Actor)
	{
		bCanSeeTarget = false;
		LastKnownTargetLocation = Actor->GetActorLocation();
		LastSeenTargetTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	}
}

void ASOTMCousinAIController::EvaluateBehavior()
{
	if (!GetPawn() || CurrentState == ESOTMCousinAIState::Disabled || CurrentState == ESOTMCousinAIState::Catch)
	{
		return;
	}

	if (AActor* Target = CurrentTarget.Get(); Target && IsValidLivingPlayer(Target))
	{
		if (bCanSeeTarget)
		{
			LastKnownTargetLocation = Target->GetActorLocation();
			LastSeenTargetTime = GetWorld()->GetTimeSeconds();
		}
		else if (GetWorld()->GetTimeSeconds() - LastSeenTargetTime > LostTargetGraceSeconds)
		{
			ClearTargetAndPatrol();
			return;
		}
		EvaluateChase();
	}
	else if (CurrentTarget.IsValid())
	{
		ClearTargetAndPatrol();
	}
}

void ASOTMCousinAIController::EvaluateChase()
{
	AActor* Target = CurrentTarget.Get();
	if (!Target || !IsValidLivingPlayer(Target))
	{
		ClearTargetAndPatrol();
		return;
	}

	if (bCanSeeTarget && GetTargetDistance() <= CatchRange && HasCatchLineOfSight(Target))
	{
		if (USOTMDemoPhase2WorldSubsystem* Phase2 = GetWorld()->GetSubsystem<USOTMDemoPhase2WorldSubsystem>();
			Phase2 && Phase2->TryStartCousinCatch(Cast<ASOTMCousinCharacter>(GetPawn()), Target))
		{
			SetState(ESOTMCousinAIState::Catch, TEXT("valid lethal catch"));
			StopMovement();
			return;
		}
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextChaseMoveTime)
	{
		NextChaseMoveTime = Now + 0.35f;
		MoveToActor(Target, CatchRange * 0.72f, true, true, true, nullptr, true);
	}
}

void ASOTMCousinAIController::SetState(const ESOTMCousinAIState NewState, const TCHAR* Reason)
{
	if (CurrentState == NewState)
	{
		return;
	}
	CurrentState = NewState;
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
	{
		if (UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
		{
			const float VariantMultiplier = PresentationVariant == 0 ? 0.88f : (PresentationVariant == 1 ? 1.12f : 1.0f);
			Movement->MaxWalkSpeed = (CurrentState == ESOTMCousinAIState::Chase ? ChaseSpeed : PatrolSpeed) * VariantMultiplier;
		}
	}
#if !UE_BUILD_SHIPPING
	UE_LOG(LogSOTMCousinAI, Display, TEXT("Cousin %s state=%d (%s)"),
		*GetNameSafe(GetPawn()), static_cast<int32>(CurrentState), Reason);
#endif
}

void ASOTMCousinAIController::BeginPatrol()
{
	if (!GetPawn())
	{
		return;
	}
	SetState(ESOTMCousinAIState::Patrol, TEXT("begin patrol"));
	QueuePatrolMove(0.10f);
}

void ASOTMCousinAIController::QueuePatrolMove(const float DelaySeconds)
{
	GetWorldTimerManager().ClearTimer(PatrolMoveTimer);
	GetWorldTimerManager().SetTimer(
		PatrolMoveTimer, this, &ThisClass::RequestPatrolMove, FMath::Max(0.01f, DelaySeconds), false);
}

void ASOTMCousinAIController::RequestPatrolMove()
{
	if (CurrentState != ESOTMCousinAIState::Patrol || !GetPawn())
	{
		return;
	}
	if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Destination;
		if (Navigation->GetRandomReachablePointInRadius(HomeLocation, PatrolRadius, Destination))
		{
			MoveToLocation(Destination.Location, 65.0f, true, true, true, true, nullptr, true);
			return;
		}
	}
	// Retry asynchronously. A synchronous MoveTo failure can immediately invoke
	// ReceiveMoveCompleted, so issuing a new request in that delegate would recurse.
	QueuePatrolMove(0.75f);
}

void ASOTMCousinAIController::HandleMoveCompleted(
	FAIRequestID RequestId,
	EPathFollowingResult::Type Result)
{
	(void)RequestId;
	(void)Result;
	if (CurrentState == ESOTMCousinAIState::Patrol)
	{
		QueuePatrolMove(0.35f);
	}
}

void ASOTMCousinAIController::ClearTargetAndPatrol()
{
	if (USOTMDemoPhase2WorldSubsystem* Phase2 = GetWorld()->GetSubsystem<USOTMDemoPhase2WorldSubsystem>())
	{
		Phase2->NotifyCousinEncounterEnded(this);
	}
	CurrentTarget.Reset();
	bCanSeeTarget = false;
	StopMovement();
	BeginPatrol();
}

void ASOTMCousinAIController::SuspendForPlayerDeath()
{
	StopMovement();
	CurrentTarget.Reset();
	bCanSeeTarget = false;
	SetState(ESOTMCousinAIState::Disabled, TEXT("player death/respawn"));
}

void ASOTMCousinAIController::ResetAfterPlayerRespawn()
{
	CurrentTarget.Reset();
	bCanSeeTarget = false;
	LastSeenTargetTime = -1.0f;
	BeginPatrol();
}

bool ASOTMCousinAIController::IsValidLivingPlayer(const AActor* Actor) const
{
	if (!Actor || !GetWorld())
	{
		return false;
	}
	const USOTMPlayerStateSubsystem* State = GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	if (!State || !State->IsBoundPlayerActor(Actor) || State->IsPlayerDead() || State->IsGameOver())
	{
		return false;
	}
	const USOTMPlayerVitalComponent* Vital = Actor->FindComponentByClass<USOTMPlayerVitalComponent>();
	return Vital && !Vital->IsDead() && !Vital->IsInvulnerable();
}

bool ASOTMCousinAIController::HasCatchLineOfSight(const AActor* Actor) const
{
	return Actor && LineOfSightTo(Actor);
}

float ASOTMCousinAIController::GetTargetDistance() const
{
	return GetPawn() && CurrentTarget.IsValid()
		? FVector::Dist(GetPawn()->GetActorLocation(), CurrentTarget->GetActorLocation())
		: TNumericLimits<float>::Max();
}
