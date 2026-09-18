#pragma once

#include "CoreMinimal.h"
#include "Demo/SOTMPhase4Types.h"
#include "GameFramework/Actor.h"
#include "SOTMPhase4Interactable.generated.h"

class USceneComponent;
class USphereComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTMPhase4OverlapEvent, ESOTMPhase4InteractableKind, AActor*);

/** Transient overlap anchor attached to existing CH1 production art. */
UCLASS(NotBlueprintable, Transient)
class SOTM1_API ASOTMPhase4Interactable final : public AActor
{
	GENERATED_BODY()

public:
	ASOTMPhase4Interactable();

	void Configure(ESOTMPhase4InteractableKind InKind, float Radius);
	ESOTMPhase4InteractableKind GetKind() const { return Kind; }

	FSOTMPhase4OverlapEvent OnPlayerEntered;
	FSOTMPhase4OverlapEvent OnPlayerExited;

private:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractionSphere;

	ESOTMPhase4InteractableKind Kind = ESOTMPhase4InteractableKind::Chest;
};
