#include "UI/SOTMPlayerHealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMPlayerHealthBar, Log, All);

namespace { constexpr int32 MaxPlayerBindingRetries = 20; }

void USOTMPlayerHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	NormalVisibility = GetVisibility();

	HealthProgressBar = Cast<UProgressBar>(
		WidgetTree ? WidgetTree->FindWidget(TEXT("ProgressBar_138")) : nullptr);
	if (!HealthProgressBar)
	{
		UE_LOG(LogSOTMPlayerHealthBar, Error,
			TEXT("Production health widget %s is missing ProgressBar_138"), *GetNameSafe(this));
		return;
	}

	BindToProductionPlayer();

	BoundPlayerState = USOTMPlayerBlueprintLibrary::GetPlayerStateSubsystem(this);
	if (BoundPlayerState)
	{
		BoundPlayerState->OnInputLocksChanged.RemoveDynamic(
			this, &ThisClass::HandleInputLocksChanged);
		BoundPlayerState->OnInputLocksChanged.AddDynamic(
			this, &ThisClass::HandleInputLocksChanged);
		HandleInputLocksChanged(
			BoundPlayerState->HasAnyInputLock(),
			BoundPlayerState->GetActiveInputLockReasons());
	}
}

void USOTMPlayerHealthBarWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerBindingRetryTimer);
	}

	if (BoundVitalComponent)
	{
		BoundVitalComponent->OnHealthChanged.RemoveDynamic(
			this, &ThisClass::HandleHealthChanged);
	}
	if (BoundPlayerState)
	{
		BoundPlayerState->OnInputLocksChanged.RemoveDynamic(
			this, &ThisClass::HandleInputLocksChanged);
	}
	BoundVitalComponent = nullptr;
	BoundPlayerState = nullptr;
	HealthProgressBar = nullptr;

	Super::NativeDestruct();
}

void USOTMPlayerHealthBarWidget::HandleInputLocksChanged(
	const bool bInputLocked,
	TArray<ESOTMInputLockReason> ActiveReasons)
{
	(void)bInputLocked;
	const bool bSuppressHealth = ActiveReasons.Contains(ESOTMInputLockReason::Death) ||
		ActiveReasons.Contains(ESOTMInputLockReason::Respawn) ||
		ActiveReasons.Contains(ESOTMInputLockReason::Cinematic) ||
		ActiveReasons.Contains(ESOTMInputLockReason::JumpScare) ||
		ActiveReasons.Contains(ESOTMInputLockReason::GameOver);
	SetVisibility(bSuppressHealth ? ESlateVisibility::Collapsed : NormalVisibility);
}

void USOTMPlayerHealthBarWidget::BindToProductionPlayer()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	APawn* PlayerPawn = GetOwningPlayerPawn();
	if (!PlayerPawn && PlayerController)
	{
		PlayerPawn = PlayerController->GetPawn().Get();
	}
	USOTMPlayerVitalComponent* VitalComponent =
		PlayerPawn ? PlayerPawn->FindComponentByClass<USOTMPlayerVitalComponent>() : nullptr;

	if (!VitalComponent)
	{
		if (UWorld* World = GetWorld();
			World && PlayerBindingRetryCount < MaxPlayerBindingRetries)
		{
			++PlayerBindingRetryCount;
			World->GetTimerManager().SetTimer(
				PlayerBindingRetryTimer,
				this,
				&ThisClass::BindToProductionPlayer,
				0.1f,
				false);
		}
		else
		{
			UE_LOG(LogSOTMPlayerHealthBar, Warning,
				TEXT("Production health widget could not bind to the Player System after %d retries"),
				PlayerBindingRetryCount);
		}
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(PlayerBindingRetryTimer);
	if (BoundVitalComponent && BoundVitalComponent != VitalComponent)
	{
		BoundVitalComponent->OnHealthChanged.RemoveDynamic(
			this, &ThisClass::HandleHealthChanged);
	}

	BoundVitalComponent = VitalComponent;
	BoundVitalComponent->OnHealthChanged.RemoveDynamic(
		this, &ThisClass::HandleHealthChanged);
	BoundVitalComponent->OnHealthChanged.AddDynamic(
		this, &ThisClass::HandleHealthChanged);
	RefreshHealth(
		BoundVitalComponent->GetCurrentHealth(),
		BoundVitalComponent->GetMaximumHealth());
}

void USOTMPlayerHealthBarWidget::HandleHealthChanged(
	USOTMPlayerVitalComponent* VitalComponent,
	const float PreviousHealth,
	const float CurrentHealth,
	const float MaximumHealth)
{
	(void)PreviousHealth;

	if (VitalComponent == BoundVitalComponent)
	{
		RefreshHealth(CurrentHealth, MaximumHealth);
	}
}

void USOTMPlayerHealthBarWidget::RefreshHealth(
	const float CurrentHealth,
	const float MaximumHealth)
{
	if (HealthProgressBar)
	{
		const float NormalizedHealth = MaximumHealth > 0.0f
			? FMath::Clamp(CurrentHealth / MaximumHealth, 0.0f, 1.0f)
			: 0.0f;
		HealthProgressBar->SetPercent(NormalizedHealth);
	}
}
