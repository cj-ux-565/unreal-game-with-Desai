#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTMTimmyUpgradeStation.generated.h"

class USphereComponent;
class UTextRenderComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FSOTMStationPlayerEvent, AActor*);

/** Runtime station trigger placed around the real CH1 Timmy/HorrorBear actor. */
UCLASS(NotBlueprintable)
class SOTM1_API ASOTMTimmyUpgradeStation final : public AActor
{
	GENERATED_BODY()

public:
	ASOTMTimmyUpgradeStation();
	virtual void BeginPlay() override;

	FSOTMStationPlayerEvent OnPlayerEntered;
	FSOTMStationPlayerEvent OnPlayerExited;

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

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> StationTitle;
};
