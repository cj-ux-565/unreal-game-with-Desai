#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTMPlayerVitalComponent.generated.h"

class AController;
class USOTMPlayerVitalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FSOTMHealthChangedSignature,
	USOTMPlayerVitalComponent*, VitalComponent,
	float, PreviousHealth,
	float, CurrentHealth,
	float, MaximumHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FSOTMDamagedSignature,
	USOTMPlayerVitalComponent*, VitalComponent,
	float, AppliedDamage,
	AController*, InstigatedBy,
	AActor*, DamageCauser);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FSOTMDeathSignature,
	USOTMPlayerVitalComponent*, VitalComponent,
	AController*, InstigatedBy,
	AActor*, DamageCauser);

/**
 * Event-driven, authoritative health component. It accepts Unreal's standard
 * damage events and contains no per-frame Tick.
 */
UCLASS(ClassGroup=(SOTM), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class SOTM1_API USOTMPlayerVitalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USOTMPlayerVitalComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Vitals")
	void InitializeVitals(float InMaximumHealth, float InCurrentHealth = -1.0f);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Vitals")
	bool ApplySOTMDamage(float Damage, AController* InstigatedBy = nullptr, AActor* DamageCauser = nullptr);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Vitals")
	float Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Vitals")
	void ResetToFullHealth();

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Vitals")
	void Revive(float RestoredHealth);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Vitals")
	void SetInvulnerable(bool bNewInvulnerable);

	UFUNCTION(BlueprintCallable, Category="SOTM|Player|Vitals")
	void SetInvulnerableForDuration(float DurationSeconds);

	UFUNCTION(BlueprintPure, Category="SOTM|Player|Vitals")
	float GetMaximumHealth() const { return MaximumHealth; }

	UFUNCTION(BlueprintPure, Category="SOTM|Player|Vitals")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="SOTM|Player|Vitals")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintPure, Category="SOTM|Player|Vitals")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category="SOTM|Player|Vitals")
	bool IsInvulnerable() const { return bInvulnerable; }

	UPROPERTY(BlueprintAssignable, Category="SOTM|Player|Vitals")
	FSOTMHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="SOTM|Player|Vitals")
	FSOTMDamagedSignature OnDamaged;

	UPROPERTY(BlueprintAssignable, Category="SOTM|Player|Vitals")
	FSOTMDeathSignature OnDeath;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Player|Vitals", meta=(ClampMin="1.0"))
	float MaximumHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Player|Vitals")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Player|Vitals")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTM|Player|Vitals")
	bool bInvulnerable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Player|Vitals", meta=(ClampMin="0.0"))
	float DamageInvulnerabilitySeconds = 1.0f;

private:
	UFUNCTION()
	void HandleAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser);

	void ClearInvulnerability();
	void BroadcastHealthChanged(float PreviousHealth);

	FTimerHandle InvulnerabilityTimerHandle;
};
