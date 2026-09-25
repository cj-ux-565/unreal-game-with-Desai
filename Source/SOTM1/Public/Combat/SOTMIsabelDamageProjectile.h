#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTMIsabelDamageProjectile.generated.h"

class UParticleSystemComponent;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;

/**
 * Minimal dedicated projectile for the existing Isabel boss fight.
 * Existing LMB path spawns this instead of BP_LightingThrow (single editor
 * class swap, no graph rewrite). Damage flows through the standard
 * ApplyDamage -> pawn OnTakeAnyDamage -> existing controller bridge path;
 * this class owns no health logic.
 */
UCLASS(NotBlueprintable)
class SOTM1_API ASOTMIsabelDamageProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASOTMIsabelDamageProjectile();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	bool ShouldIgnoreActor(const AActor* OtherActor) const;

	UPROPERTY(VisibleAnywhere, Category="SOTM|Combat")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category="SOTM|Combat")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, Category="SOTM|Combat")
	TObjectPtr<UParticleSystemComponent> LightningVisual;

	static constexpr float DamageAmount = 25.0f;
	static constexpr float LifeSeconds = 2.0f;
	static constexpr float FlightSpeed = 2000.0f;
};
