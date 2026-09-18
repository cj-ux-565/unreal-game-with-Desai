#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/SOTMCousinTypes.h"
#include "SOTMCousinAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/** One timer-driven, perception-based behavior shared by every Forest Cousin. */
UCLASS(Blueprintable)
class SOTM1_API ASOTMCousinAIController final : public AAIController
{
	GENERATED_BODY()

public:
	ASOTMCousinAIController();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category="SOTM|Cousin")
	ESOTMCousinAIState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category="SOTM|Cousin")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	void SuspendForPlayerDeath();
	void ResetAfterPlayerRespawn();
	void SetPresentationVariant(int32 VariantIndex);

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestId, EPathFollowingResult::Type Result);

	void EvaluateBehavior();
	void SetState(ESOTMCousinAIState NewState, const TCHAR* Reason);
	void BeginPatrol();
	void QueuePatrolMove(float DelaySeconds);
	void RequestPatrolMove();
	void EvaluateChase();
	void ClearTargetAndPatrol();
	bool IsValidLivingPlayer(const AActor* Actor) const;
	bool HasCatchLineOfSight(const AActor* Actor) const;
	float GetTargetDistance() const;

	UPROPERTY(VisibleAnywhere, Category="SOTM|Cousin|Perception")
	TObjectPtr<UAIPerceptionComponent> CousinPerception;

	UPROPERTY(VisibleAnywhere, Category="SOTM|Cousin|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Perception", meta=(ClampMin="200.0"))
	float SightRadius = 1250.0f;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Perception", meta=(ClampMin="200.0"))
	float LoseSightRadius = 1550.0f;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Perception", meta=(ClampMin="1.0", ClampMax="180.0"))
	float PeripheralVisionHalfAngle = 65.0f;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Movement", meta=(ClampMin="50.0"))
	float PatrolSpeed = 155.0f;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Movement", meta=(ClampMin="50.0"))
	float ChaseSpeed = 390.0f;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Movement", meta=(ClampMin="100.0"))
	float PatrolRadius = 700.0f;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Catch", meta=(ClampMin="50.0"))
	float CatchRange = 180.0f;

	UPROPERTY(EditAnywhere, Category="SOTM|Cousin|Chase", meta=(ClampMin="0.2"))
	float LostTargetGraceSeconds = 3.0f;

	UPROPERTY(Transient)
	ESOTMCousinAIState CurrentState = ESOTMCousinAIState::Idle;

	TWeakObjectPtr<AActor> CurrentTarget;
	FVector HomeLocation = FVector::ZeroVector;
	FVector LastKnownTargetLocation = FVector::ZeroVector;
	float LastSeenTargetTime = -1.0f;
	float NextChaseMoveTime = 0.0f;
	int32 PresentationVariant = 0;
	bool bCanSeeTarget = false;
	FTimerHandle EvaluationTimer;
	FTimerHandle PatrolMoveTimer;
};
