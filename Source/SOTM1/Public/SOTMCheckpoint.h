#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTMCheckpoint.generated.h"

class UBoxComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTMCheckpointActivatedSignature, FName, CheckpointId, AActor*, PlayerActor);

/**
 * Reusable, art-agnostic checkpoint trigger. Designers can bind visual/audio
 * feedback to OnCheckpointActivated without changing checkpoint logic.
 */
UCLASS(Blueprintable)
class SOTM1_API ASOTMCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	ASOTMCheckpoint();

	UFUNCTION(BlueprintCallable, Category="SOTM|Checkpoint")
	bool ActivateCheckpoint(AActor* PlayerActor);

	UFUNCTION(BlueprintCallable, Category="SOTM|Checkpoint")
	void ResetCheckpoint();

	UFUNCTION(BlueprintPure, Category="SOTM|Checkpoint")
	bool IsActivated() const { return bActivated; }

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="SOTM|Checkpoint")
	FName CheckpointId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Checkpoint")
	bool bActivateOnlyOnce = true;

	UPROPERTY(BlueprintAssignable, Category="SOTM|Checkpoint")
	FSOTMCheckpointActivatedSignature OnCheckpointActivated;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Checkpoint")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Checkpoint")
	TObjectPtr<USceneComponent> RespawnPoint;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="SOTM|Checkpoint")
	bool bActivated = false;

private:
	FName GetNormalizedMapPackageName() const;
};
