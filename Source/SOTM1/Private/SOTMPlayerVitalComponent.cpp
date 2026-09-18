#include "SOTMPlayerVitalComponent.h"

#include "GameFramework/Actor.h"
#include "SOTMPlayerSystemSettings.h"
#include "TimerManager.h"

USOTMPlayerVitalComponent::USOTMPlayerVitalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	const USOTMPlayerSystemSettings* Settings = GetDefault<USOTMPlayerSystemSettings>();
	MaximumHealth = FMath::Max(1.0f, Settings->MaximumHealth);
	CurrentHealth = MaximumHealth;
	DamageInvulnerabilitySeconds = FMath::Max(0.0f, Settings->DamageInvulnerabilitySeconds);
}

void USOTMPlayerVitalComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &USOTMPlayerVitalComponent::HandleAnyDamage);
	}
}

void USOTMPlayerVitalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &USOTMPlayerVitalComponent::HandleAnyDamage);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvulnerabilityTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void USOTMPlayerVitalComponent::InitializeVitals(const float InMaximumHealth, const float InCurrentHealth)
{
	const float PreviousHealth = CurrentHealth;
	MaximumHealth = FMath::Max(1.0f, InMaximumHealth);
	CurrentHealth = FMath::Clamp(InCurrentHealth < 0.0f ? MaximumHealth : InCurrentHealth, 0.0f, MaximumHealth);
	bIsDead = CurrentHealth <= 0.0f;
	bInvulnerable = false;
	BroadcastHealthChanged(PreviousHealth);
}

bool USOTMPlayerVitalComponent::ApplySOTMDamage(
	const float Damage,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	if (Damage <= 0.0f || bInvulnerable || bIsDead)
	{
		return false;
	}

	const float PreviousHealth = CurrentHealth;
	const float AppliedDamage = FMath::Min(Damage, CurrentHealth);
	CurrentHealth = FMath::Clamp(CurrentHealth - AppliedDamage, 0.0f, MaximumHealth);

	OnDamaged.Broadcast(this, AppliedDamage, InstigatedBy, DamageCauser);
	BroadcastHealthChanged(PreviousHealth);

	if (CurrentHealth <= 0.0f)
	{
		// Set the guard before broadcasting so re-entrant damage cannot process death twice.
		bIsDead = true;
		bInvulnerable = true;
		OnDeath.Broadcast(this, InstigatedBy, DamageCauser);
		return true;
	}

	SetInvulnerableForDuration(DamageInvulnerabilitySeconds);
	return true;
}

float USOTMPlayerVitalComponent::Heal(const float Amount)
{
	if (Amount <= 0.0f || bIsDead)
	{
		return 0.0f;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaximumHealth);
	BroadcastHealthChanged(PreviousHealth);
	return CurrentHealth - PreviousHealth;
}

void USOTMPlayerVitalComponent::ResetToFullHealth()
{
	Revive(MaximumHealth);
}

void USOTMPlayerVitalComponent::Revive(const float RestoredHealth)
{
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(RestoredHealth, 1.0f, MaximumHealth);
	bIsDead = false;
	bInvulnerable = false;
	BroadcastHealthChanged(PreviousHealth);
}

void USOTMPlayerVitalComponent::SetInvulnerable(const bool bNewInvulnerable)
{
	bInvulnerable = bNewInvulnerable;

	if (!bInvulnerable)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(InvulnerabilityTimerHandle);
		}
	}
}

void USOTMPlayerVitalComponent::SetInvulnerableForDuration(const float DurationSeconds)
{
	if (bIsDead)
	{
		return;
	}

	bInvulnerable = DurationSeconds > 0.0f;

	if (!bInvulnerable)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InvulnerabilityTimerHandle,
			this,
			&USOTMPlayerVitalComponent::ClearInvulnerability,
			DurationSeconds,
			false);
	}
}

float USOTMPlayerVitalComponent::GetHealthNormalized() const
{
	return MaximumHealth > 0.0f ? CurrentHealth / MaximumHealth : 0.0f;
}

void USOTMPlayerVitalComponent::HandleAnyDamage(
	AActor* DamagedActor,
	const float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	ApplySOTMDamage(Damage, InstigatedBy, DamageCauser);
}

void USOTMPlayerVitalComponent::ClearInvulnerability()
{
	if (!bIsDead)
	{
		bInvulnerable = false;
	}
}

void USOTMPlayerVitalComponent::BroadcastHealthChanged(const float PreviousHealth)
{
	if (!FMath::IsNearlyEqual(PreviousHealth, CurrentHealth))
	{
		OnHealthChanged.Broadcast(this, PreviousHealth, CurrentHealth, MaximumHealth);
	}
}
