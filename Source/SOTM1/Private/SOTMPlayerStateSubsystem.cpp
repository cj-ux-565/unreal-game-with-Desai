#include "SOTMPlayerStateSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "SOTMGameOverWidget.h"
#include "SOTMPlayerVitalComponent.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace SOTMPlayerStatePrivate
{
	const FName HasStateProperty(TEXT("SOTM_HasPlayerState"));
	const FName SaveVersionProperty(TEXT("SOTM_SaveVersion"));
	const FName CurrentLivesProperty(TEXT("SOTM_CurrentLives"));
	const FName MaximumLivesProperty(TEXT("SOTM_MaximumLives"));
	const FName CurrentHealthProperty(TEXT("SOTM_CurrentHealth"));
	const FName CheckpointIdProperty(TEXT("SOTM_CheckpointId"));
	const FName CheckpointMapProperty(TEXT("SOTM_CheckpointMap"));
	const FName CheckpointTransformProperty(TEXT("SOTM_CheckpointTransform"));
	const FName AvailableCoinsProperty(TEXT("SOTM_AvailableCoins"));
	const FName LifetimeCoinsCollectedProperty(TEXT("SOTM_LifetimeCoinsCollected"));
	const FName CollectedCoinIdsProperty(TEXT("SOTM_CollectedCoinIds"));
	const FString SpeedBoostRecordPrefix(TEXT("SOTM_SPEEDBOOST|"));
	const FString Phase4RecordPrefix(TEXT("SOTM_PHASE4|"));

	bool GetBoolProperty(const UObject* Object, const FName Name, bool& OutValue)
	{
		if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), Name))
		{
			OutValue = Property->GetPropertyValue_InContainer(Object);
			return true;
		}
		return false;
	}

	bool SetBoolProperty(UObject* Object, const FName Name, const bool Value)
	{
		if (FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), Name))
		{
			Property->SetPropertyValue_InContainer(Object, Value);
			return true;
		}
		return false;
	}

	bool GetIntProperty(const UObject* Object, const FName Name, int32& OutValue)
	{
		if (const FIntProperty* Property = FindFProperty<FIntProperty>(Object->GetClass(), Name))
		{
			OutValue = Property->GetPropertyValue_InContainer(Object);
			return true;
		}
		return false;
	}

	bool SetIntProperty(UObject* Object, const FName Name, const int32 Value)
	{
		if (FIntProperty* Property = FindFProperty<FIntProperty>(Object->GetClass(), Name))
		{
			Property->SetPropertyValue_InContainer(Object, Value);
			return true;
		}
		return false;
	}

	bool GetFloatProperty(const UObject* Object, const FName Name, float& OutValue)
	{
		const FNumericProperty* NumericProperty =
			CastField<FNumericProperty>(FindFProperty<FProperty>(Object->GetClass(), Name));
		if (!NumericProperty)
		{
			return false;
		}

		const void* ValueAddress = NumericProperty->ContainerPtrToValuePtr<void>(Object);
		if (NumericProperty->IsFloatingPoint())
		{
			OutValue = static_cast<float>(NumericProperty->GetFloatingPointPropertyValue(ValueAddress));
		}
		else
		{
			OutValue = static_cast<float>(NumericProperty->GetSignedIntPropertyValue(ValueAddress));
		}
		return true;
	}

	bool SetFloatProperty(UObject* Object, const FName Name, const float Value)
	{
		FNumericProperty* NumericProperty =
			CastField<FNumericProperty>(FindFProperty<FProperty>(Object->GetClass(), Name));
		if (!NumericProperty)
		{
			return false;
		}

		void* ValueAddress = NumericProperty->ContainerPtrToValuePtr<void>(Object);
		if (NumericProperty->IsFloatingPoint())
		{
			NumericProperty->SetFloatingPointPropertyValue(ValueAddress, static_cast<double>(Value));
		}
		else
		{
			NumericProperty->SetIntPropertyValue(ValueAddress, FMath::RoundToInt64(Value));
		}
		return true;
	}

	bool GetNameProperty(const UObject* Object, const FName Name, FName& OutValue)
	{
		if (const FNameProperty* Property = FindFProperty<FNameProperty>(Object->GetClass(), Name))
		{
			OutValue = Property->GetPropertyValue_InContainer(Object);
			return true;
		}
		return false;
	}

	bool SetNameProperty(UObject* Object, const FName Name, const FName Value)
	{
		if (FNameProperty* Property = FindFProperty<FNameProperty>(Object->GetClass(), Name))
		{
			Property->SetPropertyValue_InContainer(Object, Value);
			return true;
		}
		return false;
	}

	bool GetTransformProperty(const UObject* Object, const FName Name, FTransform& OutValue)
	{
		if (const FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), Name))
		{
			if (Property->Struct == TBaseStructure<FTransform>::Get())
			{
				OutValue = *Property->ContainerPtrToValuePtr<FTransform>(Object);
				return true;
			}
		}
		return false;
	}

	bool SetTransformProperty(UObject* Object, const FName Name, const FTransform& Value)
	{
		if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), Name))
		{
			if (Property->Struct == TBaseStructure<FTransform>::Get())
			{
				*Property->ContainerPtrToValuePtr<FTransform>(Object) = Value;
				return true;
			}
		}
		return false;
	}

	bool GetStringProperty(const UObject* Object, const FName Name, FString& OutValue)
	{
		if (const FStrProperty* Property = FindFProperty<FStrProperty>(Object->GetClass(), Name))
		{
			OutValue = Property->GetPropertyValue_InContainer(Object);
			return true;
		}
		return false;
	}

	bool SetStringProperty(UObject* Object, const FName Name, const FString& Value)
	{
		if (FStrProperty* Property = FindFProperty<FStrProperty>(Object->GetClass(), Name))
		{
			Property->SetPropertyValue_InContainer(Object, Value);
			return true;
		}
		return false;
	}
}

void USOTMPlayerStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const USOTMPlayerSystemSettings* Settings = GetDefault<USOTMPlayerSystemSettings>();
	MaximumLives = FMath::Max(1, Settings->MaximumLives);
	CurrentLives = FMath::Clamp(Settings->StartingLives, 1, MaximumLives);
	PersistentHealth = FMath::Max(1.0f, Settings->MaximumHealth);
}

void USOTMPlayerStateSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
		World->GetTimerManager().ClearTimer(GameOverTimerHandle);
	}

	if (BoundVitalComponent)
	{
		BoundVitalComponent->OnHealthChanged.RemoveDynamic(this, &USOTMPlayerStateSubsystem::HandleHealthChanged);
		BoundVitalComponent->OnDeath.RemoveDynamic(this, &USOTMPlayerStateSubsystem::HandlePlayerDeath);
	}

	ClearAllInputLocks();
	HideGameOverWidget();
	Super::Deinitialize();
}

void USOTMPlayerStateSubsystem::PrepareForGameplayWorld(const FName MapPackageName)
{
	CurrentGameplayMap = MapPackageName;

	const USOTMPlayerSystemSettings* Settings = GetDefault<USOTMPlayerSystemSettings>();
	// The production menu chooses/loads its authoritative save only after New Game
	// or Continue is pressed. Consuming the one-time auto-load in Main_Menu_Map read
	// the manager's placeholder object (and its default "Slot 1") too early.
	if (MapPackageName == Settings->MainMenuMap)
	{
		return;
	}

	if (Settings->bAutoLoadPlayerState)
	{
		SynchronizeFromActiveMenuSave(TEXT("GameplayWorld"), true);
	}
}

void USOTMPlayerStateSubsystem::BindPlayer(AActor* PlayerActor, USOTMPlayerVitalComponent* VitalComponent)
{
	if (!IsValid(PlayerActor) || !IsValid(VitalComponent))
	{
		return;
	}

	if (BoundVitalComponent && BoundVitalComponent != VitalComponent)
	{
		BoundVitalComponent->OnHealthChanged.RemoveDynamic(this, &USOTMPlayerStateSubsystem::HandleHealthChanged);
		BoundVitalComponent->OnDeath.RemoveDynamic(this, &USOTMPlayerStateSubsystem::HandlePlayerDeath);
	}

	// Continue finishes loading through Menu System Pro's loading-screen callback.
	// Re-check at gameplay-pawn binding so the HUD/vitals cannot bind to the
	// pre-Continue placeholder state if World BeginPlay happened first. The menu
	// preview pawn must never be interpreted as a New Game.
	if (CurrentGameplayMap != GetDefault<USOTMPlayerSystemSettings>()->MainMenuMap)
	{
		SynchronizeFromActiveMenuSave(TEXT("PlayerBind"), true);
	}

	BoundPlayerActor = PlayerActor;
	BoundVitalComponent = VitalComponent;

	BoundVitalComponent->OnHealthChanged.RemoveDynamic(this, &USOTMPlayerStateSubsystem::HandleHealthChanged);
	BoundVitalComponent->OnDeath.RemoveDynamic(this, &USOTMPlayerStateSubsystem::HandlePlayerDeath);
	BoundVitalComponent->OnHealthChanged.AddDynamic(this, &USOTMPlayerStateSubsystem::HandleHealthChanged);
	BoundVitalComponent->OnDeath.AddDynamic(this, &USOTMPlayerStateSubsystem::HandlePlayerDeath);

	ApplyLoadedStateToBoundPlayer();
	EnsureAutomaticMapEntryCheckpoint();
	ApplyInputLockState();

	if (bPendingRespawnAfterTravel)
	{
		bPendingRespawnAfterTravel = false;
		PerformRespawn();
	}
}

bool USOTMPlayerStateSubsystem::TryCollectCoin(
	const FGuid PersistentCoinId,
	AActor* Collector,
	const int32 CoinValue)
{
	if (!PersistentCoinId.IsValid() ||
		CollectedCoinIds.Contains(PersistentCoinId) ||
		!IsBoundPlayerActor(Collector) ||
		CoinValue <= 0 ||
		bPlayerDead ||
		bGameOver)
	{
		return false;
	}

	CollectedCoinIds.Add(PersistentCoinId);
	AvailableCoins = FMath::Max(0, AvailableCoins + CoinValue);
	LifetimeCoinsCollected = FMath::Max(0, LifetimeCoinsCollected + CoinValue);
	bCoinStateDirty = true;
	OnCoinCollected.Broadcast(CoinValue, AvailableCoins);
	OnCoinsChanged.Broadcast(AvailableCoins, LifetimeCoinsCollected);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("SOTM Coin: collected id=%s value=%d available=%d lifetime=%d"),
		*PersistentCoinId.ToString(EGuidFormats::DigitsWithHyphens),
		CoinValue,
		AvailableCoins,
		LifetimeCoinsCollected);

	// Persist only after the authoritative transaction has succeeded. SavePlayerState
	// resolves the active Menu System Pro slot used by Continue, so a player may quit
	// immediately after a pickup without losing the new total or collected Coin ID.
	if (!SavePlayerStateInternal(TEXT("CoinPickup")))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SOTM Coin: autosave failed after collecting id=%s; runtime state remains valid but is not guaranteed on relaunch."),
			*PersistentCoinId.ToString(EGuidFormats::DigitsWithHyphens));
	}
	return true;
}

ESOTMSpeedBoostPurchaseResult USOTMPlayerStateSubsystem::TryPurchaseSpeedBoost(
	const int32 UnlockCost,
	const bool bCoinObjectiveCompleted)
{
	if (bSpeedBoostUnlocked || SpeedBoostLevel > 0)
	{
		return ESOTMSpeedBoostPurchaseResult::AlreadyOwned;
	}
	if (!bCoinObjectiveCompleted)
	{
		return ESOTMSpeedBoostPurchaseResult::ObjectiveIncomplete;
	}
	if (UnlockCost <= 0)
	{
		return ESOTMSpeedBoostPurchaseResult::InvalidCost;
	}
	if (AvailableCoins < UnlockCost)
	{
		return ESOTMSpeedBoostPurchaseResult::NotEnoughCoins;
	}

	UObject* Manager = nullptr;
	USaveGame* SaveObject = nullptr;
	FString SlotName;
	if (!GetMenuSaveContext(Manager, SaveObject, SlotName) || SlotName.IsEmpty())
	{
		return ESOTMSpeedBoostPurchaseResult::NoActiveSave;
	}

	const int32 PreviousAvailableCoins = AvailableCoins;
	AvailableCoins -= UnlockCost;
	bSpeedBoostUnlocked = true;
	SpeedBoostLevel = 1;

	// Persist the complete transaction before announcing it. If the production
	// slot cannot save, roll back every runtime field so a failed click is atomic.
	if (!SavePlayerStateInternal(TEXT("SpeedBoostPurchase")))
	{
		AvailableCoins = PreviousAvailableCoins;
		bSpeedBoostUnlocked = false;
		SpeedBoostLevel = 0;
		return ESOTMSpeedBoostPurchaseResult::SaveFailed;
	}

	OnCoinsChanged.Broadcast(AvailableCoins, LifetimeCoinsCollected);
	OnSpeedBoostOwnershipChanged.Broadcast(bSpeedBoostUnlocked, SpeedBoostLevel);
	UE_LOG(LogTemp, Display,
		TEXT("SOTM Speed Boost purchased slot=\"%s\" cost=%d available=%d lifetime=%d level=%d"),
		*SlotName, UnlockCost, AvailableCoins, LifetimeCoinsCollected, SpeedBoostLevel);
	return ESOTMSpeedBoostPurchaseResult::Success;
}

bool USOTMPlayerStateSubsystem::CommitPhase4ChestOpenedAndKey()
{
	if (bPhase4ChestOpened || bPhase4HasGateKey)
	{
		return bPhase4ChestOpened && bPhase4HasGateKey;
	}
	const bool bPreviousChest = bPhase4ChestOpened;
	const bool bPreviousKey = bPhase4HasGateKey;
	bPhase4ChestOpened = true;
	bPhase4HasGateKey = true;
	if (!SavePlayerStateInternal(TEXT("Phase4ChestOpened")))
	{
		bPhase4ChestOpened = bPreviousChest;
		bPhase4HasGateKey = bPreviousKey;
		return false;
	}
	OnPhase4ProgressChanged.Broadcast(
		bPhase4ChestOpened, bPhase4HasGateKey, bPhase4GateUnlocked, bPhase4DemoCompleted);
	return true;
}

bool USOTMPlayerStateSubsystem::CommitPhase4GateUnlocked()
{
	if (bPhase4GateUnlocked)
	{
		return true;
	}
	bPhase4GateUnlocked = true;
	if (!SavePlayerStateInternal(TEXT("Phase4GateUnlocked")))
	{
		bPhase4GateUnlocked = false;
		return false;
	}
	OnPhase4ProgressChanged.Broadcast(
		bPhase4ChestOpened, bPhase4HasGateKey, bPhase4GateUnlocked, bPhase4DemoCompleted);
	return true;
}

bool USOTMPlayerStateSubsystem::CommitPhase4DemoCompleted()
{
	if (bPhase4DemoCompleted)
	{
		return true;
	}
	bPhase4DemoCompleted = true;
	if (!SavePlayerStateInternal(TEXT("Phase4DemoCompleted")))
	{
		bPhase4DemoCompleted = false;
		return false;
	}
	OnPhase4ProgressChanged.Broadcast(
		bPhase4ChestOpened, bPhase4HasGateKey, bPhase4GateUnlocked, bPhase4DemoCompleted);
	return true;
}

bool USOTMPlayerStateSubsystem::IsCoinCollected(const FGuid PersistentCoinId) const
{
	return PersistentCoinId.IsValid() && CollectedCoinIds.Contains(PersistentCoinId);
}

bool USOTMPlayerStateSubsystem::IsBoundPlayerActor(const AActor* Actor) const
{
	return IsValid(Actor) && Actor == BoundPlayerActor.Get();
}

bool USOTMPlayerStateSubsystem::ActivateCheckpoint(
	const FName CheckpointId,
	const FName MapPackageName,
	const FTransform& RespawnTransform,
	const bool bSaveImmediately)
{
	if (CheckpointId.IsNone() || MapPackageName.IsNone())
	{
		return false;
	}

	if (CheckpointState.bIsValid &&
		CheckpointState.CheckpointId == CheckpointId &&
		CheckpointState.MapPackageName == MapPackageName)
	{
		return false;
	}

	CheckpointState.bIsValid = true;
	CheckpointState.CheckpointId = CheckpointId;
	CheckpointState.MapPackageName = MapPackageName;
	CheckpointState.RespawnTransform = RespawnTransform;
	OnCheckpointChanged.Broadcast(CheckpointId, MapPackageName, RespawnTransform);

	if (bSaveImmediately && GetDefault<USOTMPlayerSystemSettings>()->bAutoSaveOnCheckpoint)
	{
		SavePlayerState();
	}

	return true;
}

bool USOTMPlayerStateSubsystem::ForceRespawnAtCheckpoint()
{
	if (!CheckpointState.bIsValid || !BoundPlayerActor || !BoundVitalComponent)
	{
		return false;
	}

	if (CheckpointState.MapPackageName != CurrentGameplayMap)
	{
		bPendingRespawnAfterTravel = true;
		UGameplayStatics::OpenLevel(this, CheckpointState.MapPackageName);
		return true;
	}

	PerformRespawn();
	return true;
}

bool USOTMPlayerStateSubsystem::RetryFromGameOver()
{
	if (!bGameOver)
	{
		return false;
	}

	const USOTMPlayerSystemSettings* Settings = GetDefault<USOTMPlayerSystemSettings>();
	if (Settings->RetryPolicy == ESOTMGameOverRetryPolicy::Disabled)
	{
		return false;
	}

	HideGameOverWidget();
	bGameOver = false;
	CurrentLives = Settings->RetryPolicy == ESOTMGameOverRetryPolicy::RestoreOneLifeAtCheckpoint
		? 1
		: MaximumLives;
	OnLivesChanged.Broadcast(CurrentLives, MaximumLives);
	ClearInputLock(ESOTMInputLockReason::GameOver);
	return ForceRespawnAtCheckpoint();
}

void USOTMPlayerStateSubsystem::ReturnToMainMenu()
{
	HideGameOverWidget();
	bGameOver = false;
	bPlayerDead = false;
	bDeathProcessing = false;
	ClearAllInputLocks();

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, GetDefault<USOTMPlayerSystemSettings>()->MainMenuMap);
	}
}

void USOTMPlayerStateSubsystem::AcquireInputLock(const ESOTMInputLockReason Reason)
{
	int32& Count = InputLockCounts.FindOrAdd(Reason);
	++Count;
	ApplyInputLockState();
}

void USOTMPlayerStateSubsystem::ReleaseInputLock(const ESOTMInputLockReason Reason)
{
	if (int32* Count = InputLockCounts.Find(Reason))
	{
		*Count = FMath::Max(0, *Count - 1);
		if (*Count == 0)
		{
			InputLockCounts.Remove(Reason);
		}
	}
	ApplyInputLockState();
}

void USOTMPlayerStateSubsystem::ClearInputLock(const ESOTMInputLockReason Reason)
{
	InputLockCounts.Remove(Reason);
	ApplyInputLockState();
}

void USOTMPlayerStateSubsystem::ClearAllInputLocks()
{
	InputLockCounts.Reset();
	ApplyInputLockState();
}

bool USOTMPlayerStateSubsystem::HasAnyInputLock() const
{
	return InputLockCounts.Num() > 0;
}

int32 USOTMPlayerStateSubsystem::GetInputLockCount(const ESOTMInputLockReason Reason) const
{
	return InputLockCounts.FindRef(Reason);
}

TArray<ESOTMInputLockReason> USOTMPlayerStateSubsystem::GetActiveInputLockReasons() const
{
	TArray<ESOTMInputLockReason> Reasons;
	InputLockCounts.GetKeys(Reasons);
	return Reasons;
}

bool USOTMPlayerStateSubsystem::SavePlayerState()
{
	return SavePlayerStateInternal(TEXT("Explicit"));
}

bool USOTMPlayerStateSubsystem::SavePlayerStateInternal(const TCHAR* Reason)
{
	UObject* Manager = nullptr;
	USaveGame* SaveObject = nullptr;
	FString SlotName;
	if (!GetMenuSaveContext(Manager, SaveObject, SlotName) || !WriteStateToSaveObject(SaveObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("SOTM Player State: menu SaveGame object is not ready; player state was not saved."));
		return false;
	}

	bool bSaved = false;
	if (UFunction* SaveFunction = Manager->FindFunction(TEXT("SaveGame")))
	{
		struct FSaveGameParameters
		{
			FString SlotName;
			int32 UserIndex = 0;
			bool bSuccess = false;
		};

		FSaveGameParameters Parameters;
		Parameters.SlotName = SlotName;
		Manager->ProcessEvent(SaveFunction, &Parameters);
		bSaved = Parameters.bSuccess;
	}
	else
	{
		// Compatibility fallback for a replacement manager that exposes the same
		// reflected state but no SaveGame function.
		bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
	}

	if (bSaved)
	{
		bCoinStateDirty = false;
	}

#if !UE_BUILD_SHIPPING
	const FString SaveFilename = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".sav"));
	const FDateTime Timestamp = IFileManager::Get().GetTimeStamp(*SaveFilename);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("[CoinSave] WRITE success=%d slot=\"%s\" user=0 class=%s available=%d lifetime=%d ids=%d reason=%s timestamp=%s"),
		bSaved,
		*SlotName,
		*SaveObject->GetClass()->GetPathName(),
		AvailableCoins,
		LifetimeCoinsCollected,
		CollectedCoinIds.Num(),
		Reason,
		*Timestamp.ToIso8601());
#endif
	return bSaved;
}

bool USOTMPlayerStateSubsystem::LoadPlayerState(const bool bTravelToCheckpoint)
{
	UObject* Manager = nullptr;
	USaveGame* SaveObject = nullptr;
	FString SlotName;
	if (!GetMenuSaveContext(Manager, SaveObject, SlotName))
	{
		return false;
	}

	if (!ReadStateFromSaveObject(SaveObject))
	{
		return false;
	}

	bLoadedPlayerState = true;
	LastSynchronizedSaveSlot = SlotName;
	LastSynchronizedSaveObject = SaveObject;
	ApplyLoadedStateToBoundPlayer();

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Display,
		TEXT("[CoinSave] READ slot=\"%s\" user=0 class=%s available=%d lifetime=%d ids=%d reason=ExplicitLoad"),
		*SlotName, *SaveObject->GetClass()->GetPathName(), AvailableCoins,
		LifetimeCoinsCollected, CollectedCoinIds.Num());
#endif

	if (bTravelToCheckpoint &&
		CheckpointState.bIsValid &&
		!CurrentGameplayMap.IsNone() &&
		CheckpointState.MapPackageName != CurrentGameplayMap)
	{
		bPendingRespawnAfterTravel = true;
		UGameplayStatics::OpenLevel(this, CheckpointState.MapPackageName);
	}

	return true;
}

bool USOTMPlayerStateSubsystem::SavePlayerStateToSlot(const FString& SlotName)
{
	FString CleanSlotName = SlotName;
	CleanSlotName.TrimStartAndEndInline();
	if (CleanSlotName.IsEmpty())
	{
		return false;
	}

	USaveGame* SaveObject = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(CleanSlotName, 0))
	{
		SaveObject = UGameplayStatics::LoadGameFromSlot(CleanSlotName, 0);
	}

	if (!SaveObject)
	{
		UClass* SaveClass = LoadClass<USaveGame>(
			nullptr,
			TEXT("/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject.BP_CustomSaveGameObject_C"));
		if (!SaveClass)
		{
			return false;
		}
		SaveObject = UGameplayStatics::CreateSaveGameObject(SaveClass);
	}

	const bool bSaved = WriteStateToSaveObject(SaveObject) &&
		UGameplayStatics::SaveGameToSlot(SaveObject, CleanSlotName, 0);
	if (bSaved)
	{
		bCoinStateDirty = false;
	}
	return bSaved;
}

bool USOTMPlayerStateSubsystem::LoadPlayerStateFromSlot(
	const FString& SlotName,
	const bool bTravelToCheckpoint)
{
	FString CleanSlotName = SlotName;
	CleanSlotName.TrimStartAndEndInline();
	if (CleanSlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(CleanSlotName, 0))
	{
		return false;
	}

	USaveGame* SaveObject = UGameplayStatics::LoadGameFromSlot(CleanSlotName, 0);
	if (!SaveObject || !ReadStateFromSaveObject(SaveObject))
	{
		return false;
	}

	bLoadedPlayerState = true;
	ApplyLoadedStateToBoundPlayer();

	if (bTravelToCheckpoint &&
		CheckpointState.bIsValid &&
		!CurrentGameplayMap.IsNone() &&
		CheckpointState.MapPackageName != CurrentGameplayMap)
	{
		bPendingRespawnAfterTravel = true;
		UGameplayStatics::OpenLevel(this, CheckpointState.MapPackageName);
	}

	return true;
}

void USOTMPlayerStateSubsystem::ResetRuntimeStateForNewGame()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
		World->GetTimerManager().ClearTimer(GameOverTimerHandle);
	}

	const USOTMPlayerSystemSettings* Settings = GetDefault<USOTMPlayerSystemSettings>();
	MaximumLives = FMath::Max(1, Settings->MaximumLives);
	CurrentLives = FMath::Clamp(Settings->StartingLives, 1, MaximumLives);
	PersistentHealth = FMath::Max(1.0f, Settings->MaximumHealth);
	AvailableCoins = 0;
	LifetimeCoinsCollected = 0;
	bSpeedBoostUnlocked = false;
	SpeedBoostLevel = 0;
	bPhase4ChestOpened = false;
	bPhase4HasGateKey = false;
	bPhase4GateUnlocked = false;
	bPhase4DemoCompleted = false;
	CollectedCoinIds.Reset();
	bCoinStateDirty = false;
	bPlayerDead = false;
	bGameOver = false;
	bDeathProcessing = false;
	bLoadedPlayerState = false;
	bPendingMansionIntro = false;
	bHasInitializedPlayerHealth = false;
	bPendingRespawnAfterTravel = false;
	CheckpointState = FSOTMCheckpointState();
	HideGameOverWidget();
	ClearAllInputLocks();
	ApplyLoadedStateToBoundPlayer();
	OnLivesChanged.Broadcast(CurrentLives, MaximumLives);
	OnCoinsChanged.Broadcast(AvailableCoins, LifetimeCoinsCollected);
	OnSpeedBoostOwnershipChanged.Broadcast(bSpeedBoostUnlocked, SpeedBoostLevel);
	OnPhase4ProgressChanged.Broadcast(false, false, false, false);
}

bool USOTMPlayerStateSubsystem::ConsumePendingMansionIntro()
{
	const bool bWasPending = bPendingMansionIntro;
	bPendingMansionIntro = false;
	return bWasPending;
}

bool USOTMPlayerStateSubsystem::SynchronizeFromActiveMenuSave(
	const TCHAR* Reason,
	const bool bResetIfSaveHasNoPlayerState)
{
	UObject* Manager = nullptr;
	USaveGame* SaveObject = nullptr;
	FString SlotName;
	if (!GetMenuSaveContext(Manager, SaveObject, SlotName))
	{
		return false;
	}

	if (LastSynchronizedSaveObject.Get() == SaveObject && LastSynchronizedSaveSlot == SlotName)
	{
		return bLoadedPlayerState;
	}

	bAutoLoadAttempted = true;
	const bool bRead = ReadStateFromSaveObject(SaveObject);
	if (bRead)
	{
		bPendingMansionIntro = false;
		bLoadedPlayerState = true;
		ApplyLoadedStateToBoundPlayer();
	}
	else if (bResetIfSaveHasNoPlayerState)
	{
		// A newly-created production slot has no SOTM schema yet. This is the
		// authoritative New Game signal; reset here, never during generic startup.
		ResetRuntimeStateForNewGame();
		bPendingMansionIntro = true;
	}

	LastSynchronizedSaveSlot = SlotName;
	LastSynchronizedSaveObject = SaveObject;

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Display,
		TEXT("[CoinSave] READ success=%d slot=\"%s\" user=0 class=%s available=%d lifetime=%d ids=%d reason=%s%s"),
		bRead,
		*SlotName,
		*SaveObject->GetClass()->GetPathName(),
		AvailableCoins,
		LifetimeCoinsCollected,
		CollectedCoinIds.Num(),
		Reason,
		(!bRead && bResetIfSaveHasNoPlayerState) ? TEXT(" (NewGameReset)") : TEXT(""));
#endif
	return bRead;
}

void USOTMPlayerStateSubsystem::SetLivesForDebug(const int32 NewLives)
{
	CurrentLives = FMath::Clamp(NewLives, 0, MaximumLives);
	OnLivesChanged.Broadcast(CurrentLives, MaximumLives);
}

void USOTMPlayerStateSubsystem::SetPhase3ProgressForDebug(
	const int32 NewAvailableCoins,
	const int32 NewLifetimeCoins,
	const bool bUnlockSpeedBoost,
	const int32 NewSpeedBoostLevel)
{
#if UE_BUILD_SHIPPING
	(void)NewAvailableCoins;
	(void)NewLifetimeCoins;
	(void)bUnlockSpeedBoost;
	(void)NewSpeedBoostLevel;
#else
	AvailableCoins = FMath::Max(0, NewAvailableCoins);
	LifetimeCoinsCollected = FMath::Max(AvailableCoins, NewLifetimeCoins);
	bSpeedBoostUnlocked = bUnlockSpeedBoost && NewSpeedBoostLevel > 0;
	SpeedBoostLevel = bSpeedBoostUnlocked ? FMath::Max(1, NewSpeedBoostLevel) : 0;
	OnCoinsChanged.Broadcast(AvailableCoins, LifetimeCoinsCollected);
	OnSpeedBoostOwnershipChanged.Broadcast(bSpeedBoostUnlocked, SpeedBoostLevel);
#endif
}

void USOTMPlayerStateSubsystem::SetPhase4ProgressForDebug(
	const bool bChestOpened,
	const bool bHasGateKey,
	const bool bGateUnlocked,
	const bool bDemoCompleted)
{
#if UE_BUILD_SHIPPING
	(void)bChestOpened;
	(void)bHasGateKey;
	(void)bGateUnlocked;
	(void)bDemoCompleted;
#else
	bPhase4ChestOpened = bChestOpened;
	bPhase4HasGateKey = bHasGateKey || bChestOpened;
	bPhase4GateUnlocked = bGateUnlocked;
	bPhase4DemoCompleted = bDemoCompleted;
	OnPhase4ProgressChanged.Broadcast(
		bPhase4ChestOpened, bPhase4HasGateKey, bPhase4GateUnlocked, bPhase4DemoCompleted);
#endif
}

void USOTMPlayerStateSubsystem::HandleHealthChanged(
	USOTMPlayerVitalComponent* VitalComponent,
	const float PreviousHealth,
	const float CurrentHealth,
	const float InMaximumHealth)
{
	PersistentHealth = CurrentHealth;
	SyncLegacyCharacterState();
}

void USOTMPlayerStateSubsystem::HandlePlayerDeath(
	USOTMPlayerVitalComponent* VitalComponent,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	if (bDeathProcessing || bGameOver)
	{
		return;
	}

	bDeathProcessing = true;
	bPlayerDead = true;
	PersistentHealth = 0.0f;
	AcquireInputLock(ESOTMInputLockReason::Death);

	if (ACharacter* Character = Cast<ACharacter>(BoundPlayerActor))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}

	CurrentLives = FMath::Max(0, CurrentLives - 1);
	OnLivesChanged.Broadcast(CurrentLives, MaximumLives);
	OnPlayerDeathStarted.Broadcast(BoundPlayerActor);
	SyncLegacyCharacterState();

	if (GetDefault<USOTMPlayerSystemSettings>()->bAutoSaveOnDeath)
	{
		SavePlayerState();
	}

	if (CurrentLives <= 0)
	{
		ScheduleGameOver();
	}
	else
	{
		ScheduleRespawn();
	}
}

void USOTMPlayerStateSubsystem::ScheduleRespawn()
{
	if (UWorld* World = GetWorld())
	{
		const float Delay = FMath::Max(0.0f, GetDefault<USOTMPlayerSystemSettings>()->RespawnDelaySeconds);
		World->GetTimerManager().SetTimer(
			RespawnTimerHandle,
			this,
			&USOTMPlayerStateSubsystem::PerformRespawn,
			FMath::Max(0.01f, Delay),
			false);
	}
}

void USOTMPlayerStateSubsystem::PerformRespawn()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameOverTimerHandle);
	}

	if (!CheckpointState.bIsValid || !BoundPlayerActor || !BoundVitalComponent)
	{
		return;
	}

	if (CheckpointState.MapPackageName != CurrentGameplayMap)
	{
		bPendingRespawnAfterTravel = true;
		UGameplayStatics::OpenLevel(this, CheckpointState.MapPackageName);
		return;
	}

	AcquireInputLock(ESOTMInputLockReason::Respawn);
	BoundPlayerActor->SetActorTransform(
		CheckpointState.RespawnTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (ACharacter* Character = Cast<ACharacter>(BoundPlayerActor))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Walking);
		}
	}

	const USOTMPlayerSystemSettings* Settings = GetDefault<USOTMPlayerSystemSettings>();
	const float RespawnHealth = Settings->bRespawnWithFullHealth
		? BoundVitalComponent->GetMaximumHealth()
		: BoundVitalComponent->GetMaximumHealth() * FMath::Clamp(Settings->RespawnHealthFraction, 0.01f, 1.0f);
	BoundVitalComponent->Revive(RespawnHealth);
	BoundVitalComponent->SetInvulnerableForDuration(Settings->RespawnSafetyInvulnerabilitySeconds);
	PersistentHealth = RespawnHealth;
	bPlayerDead = false;
	bDeathProcessing = false;

	ClearInputLock(ESOTMInputLockReason::Death);
	ClearInputLock(ESOTMInputLockReason::Respawn);
	SyncLegacyCharacterState();
	OnPlayerRespawned.Broadcast(BoundPlayerActor);
	SavePlayerState();
}

void USOTMPlayerStateSubsystem::ScheduleGameOver()
{
	if (UWorld* World = GetWorld())
	{
		const float PresentationSeconds = FMath::Max(0.01f,
			GetDefault<USOTMPlayerSystemSettings>()->RespawnDelaySeconds);
		World->GetTimerManager().SetTimer(
			GameOverTimerHandle,
			this,
			&USOTMPlayerStateSubsystem::TriggerGameOver,
			PresentationSeconds,
			false);
		UE_LOG(LogTemp, Display,
			TEXT("SOTM Player State: final-life Game Over UI delayed %.2fs for death presentation."),
			PresentationSeconds);
	}
}

void USOTMPlayerStateSubsystem::TriggerGameOver()
{
	if (bGameOver)
	{
		return;
	}

	bGameOver = true;
	AcquireInputLock(ESOTMInputLockReason::GameOver);
	ShowGameOverWidget();
	OnGameOver.Broadcast();
}

void USOTMPlayerStateSubsystem::ShowGameOverWidget()
{
	if (GameOverWidget || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	GameOverWidget = CreateWidget<USOTMGameOverWidget>(PlayerController, USOTMGameOverWidget::StaticClass());
	if (GameOverWidget)
	{
		GameOverWidget->AddToViewport(10000);
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
}

void USOTMPlayerStateSubsystem::HideGameOverWidget()
{
	if (GameOverWidget)
	{
		GameOverWidget->RemoveFromParent();
		GameOverWidget = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}

void USOTMPlayerStateSubsystem::ApplyInputLockState()
{
	APlayerController* CurrentController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;

	if (bInputBlockApplied && InputLockedController.IsValid() && InputLockedController.Get() != CurrentController)
	{
		InputLockedController->SetIgnoreMoveInput(false);
		InputLockedController->SetIgnoreLookInput(false);
		bInputBlockApplied = false;
	}

	const bool bShouldBlock = HasAnyInputLock() && CurrentController;
	if (bShouldBlock && !bInputBlockApplied)
	{
		CurrentController->SetIgnoreMoveInput(true);
		CurrentController->SetIgnoreLookInput(true);
		InputLockedController = CurrentController;
		bInputBlockApplied = true;
	}
	else if (!bShouldBlock && bInputBlockApplied && InputLockedController.IsValid())
	{
		InputLockedController->SetIgnoreMoveInput(false);
		InputLockedController->SetIgnoreLookInput(false);
		InputLockedController.Reset();
		bInputBlockApplied = false;
	}

	OnInputLocksChanged.Broadcast(HasAnyInputLock(), GetActiveInputLockReasons());
}

void USOTMPlayerStateSubsystem::SyncLegacyCharacterState()
{
	if (!BoundPlayerActor)
	{
		return;
	}

	SOTMPlayerStatePrivate::SetFloatProperty(BoundPlayerActor, TEXT("HP"), PersistentHealth);
	SOTMPlayerStatePrivate::SetFloatProperty(BoundPlayerActor, TEXT("CurrentHealth"), PersistentHealth);
	SOTMPlayerStatePrivate::SetBoolProperty(BoundPlayerActor, TEXT("isdead?"), bPlayerDead);
}

void USOTMPlayerStateSubsystem::EnsureAutomaticMapEntryCheckpoint()
{
	if (!BoundPlayerActor || CurrentGameplayMap.IsNone())
	{
		return;
	}

	if (!CheckpointState.bIsValid || CheckpointState.MapPackageName != CurrentGameplayMap)
	{
		const FString ShortMapName = FPackageName::GetShortName(CurrentGameplayMap.ToString());
		const FName AutomaticId(*FString::Printf(TEXT("MapEntry_%s"), *ShortMapName));
		ActivateCheckpoint(AutomaticId, CurrentGameplayMap, BoundPlayerActor->GetActorTransform(), false);
	}
}

void USOTMPlayerStateSubsystem::ApplyLoadedStateToBoundPlayer()
{
	if (!BoundVitalComponent)
	{
		return;
	}

	const float MaximumHealth = FMath::Max(1.0f, GetDefault<USOTMPlayerSystemSettings>()->MaximumHealth);
	const bool bUsePersistentHealth = bLoadedPlayerState || bHasInitializedPlayerHealth;
	const float HealthToApply = bUsePersistentHealth && PersistentHealth > 0.0f
		? FMath::Clamp(PersistentHealth, 1.0f, MaximumHealth)
		: MaximumHealth;

	BoundVitalComponent->InitializeVitals(MaximumHealth, HealthToApply);
	PersistentHealth = HealthToApply;
	bHasInitializedPlayerHealth = true;

	if (CurrentLives <= 0)
	{
		bGameOver = false;
		bPlayerDead = true;
		bDeathProcessing = true;
		BoundVitalComponent->SetInvulnerable(true);
		TriggerGameOver();
	}
	else
	{
		bPlayerDead = false;
		bDeathProcessing = false;
	}
	SyncLegacyCharacterState();
}

bool USOTMPlayerStateSubsystem::GetMenuSaveContext(
	UObject*& OutManager,
	USaveGame*& OutSaveObject,
	FString& OutSlotName) const
{
	OutManager = nullptr;
	OutSaveObject = nullptr;
	OutSlotName = GetDefault<USOTMPlayerSystemSettings>()->FallbackSaveSlotName;

	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	const FObjectPropertyBase* ManagerProperty =
		FindFProperty<FObjectPropertyBase>(GameInstance->GetClass(), TEXT("SaveGameManager"));
	if (!ManagerProperty)
	{
		return false;
	}

	OutManager = ManagerProperty->GetObjectPropertyValue_InContainer(GameInstance);
	if (!OutManager)
	{
		return false;
	}

	if (const FStrProperty* SlotProperty =
		FindFProperty<FStrProperty>(OutManager->GetClass(), TEXT("LastLoadedSaveSlotName")))
	{
		FString ManagerSlot = SlotProperty->GetPropertyValue_InContainer(OutManager);
		ManagerSlot.TrimStartAndEndInline();
		if (!ManagerSlot.IsEmpty())
		{
			OutSlotName = ManagerSlot;
		}
	}

	const FObjectPropertyBase* SaveObjectProperty =
		FindFProperty<FObjectPropertyBase>(OutManager->GetClass(), TEXT("SaveGameObject"));
	if (!SaveObjectProperty)
	{
		return false;
	}

	OutSaveObject = Cast<USaveGame>(SaveObjectProperty->GetObjectPropertyValue_InContainer(OutManager));
	if (!OutSaveObject && UGameplayStatics::DoesSaveGameExist(OutSlotName, 0))
	{
		OutSaveObject = UGameplayStatics::LoadGameFromSlot(OutSlotName, 0);
		if (OutSaveObject)
		{
			SaveObjectProperty->SetObjectPropertyValue_InContainer(OutManager, OutSaveObject);
		}
	}

	return OutSaveObject != nullptr;
}

bool USOTMPlayerStateSubsystem::WriteStateToSaveObject(UObject* SaveObject) const
{
	if (!SaveObject)
	{
		return false;
	}

	const bool bHasStateWritten =
		SOTMPlayerStatePrivate::SetBoolProperty(SaveObject, SOTMPlayerStatePrivate::HasStateProperty, true);
	const bool bVersionWritten =
		SOTMPlayerStatePrivate::SetIntProperty(SaveObject, SOTMPlayerStatePrivate::SaveVersionProperty, CurrentSaveVersion);
	const bool bLivesWritten =
		SOTMPlayerStatePrivate::SetIntProperty(SaveObject, SOTMPlayerStatePrivate::CurrentLivesProperty, CurrentLives);
	const bool bMaximumLivesWritten =
		SOTMPlayerStatePrivate::SetIntProperty(SaveObject, SOTMPlayerStatePrivate::MaximumLivesProperty, MaximumLives);
	const bool bHealthWritten =
		SOTMPlayerStatePrivate::SetFloatProperty(SaveObject, SOTMPlayerStatePrivate::CurrentHealthProperty, PersistentHealth);
	const bool bCheckpointIdWritten =
		SOTMPlayerStatePrivate::SetNameProperty(
			SaveObject,
			SOTMPlayerStatePrivate::CheckpointIdProperty,
			CheckpointState.CheckpointId);
	const bool bCheckpointMapWritten =
		SOTMPlayerStatePrivate::SetNameProperty(
			SaveObject,
			SOTMPlayerStatePrivate::CheckpointMapProperty,
			CheckpointState.MapPackageName);
	const bool bCheckpointTransformWritten =
		SOTMPlayerStatePrivate::SetTransformProperty(
			SaveObject,
			SOTMPlayerStatePrivate::CheckpointTransformProperty,
			CheckpointState.RespawnTransform);
	const bool bAvailableCoinsWritten =
		SOTMPlayerStatePrivate::SetIntProperty(
			SaveObject,
			SOTMPlayerStatePrivate::AvailableCoinsProperty,
			AvailableCoins);
	const bool bLifetimeCoinsWritten =
		SOTMPlayerStatePrivate::SetIntProperty(
			SaveObject,
			SOTMPlayerStatePrivate::LifetimeCoinsCollectedProperty,
			LifetimeCoinsCollected);
	TArray<FGuid> SortedCoinIds = CollectedCoinIds.Array();
	SortedCoinIds.Sort([](const FGuid& Left, const FGuid& Right)
	{
		return Left.ToString(EGuidFormats::Digits) < Right.ToString(EGuidFormats::Digits);
	});
	TArray<FString> SerializedCoinIds;
	SerializedCoinIds.Reserve(SortedCoinIds.Num());
	for (const FGuid& CoinId : SortedCoinIds)
	{
		SerializedCoinIds.Add(CoinId.ToString(EGuidFormats::DigitsWithHyphens));
	}
	// The existing SaveGame Blueprint exposes one versioned persistent string for
	// stable Coin IDs. Append a namespaced Phase 3 record rather than creating a
	// second save file or requiring a destructive SaveGame Blueprint reparent.
	SerializedCoinIds.Add(FString::Printf(TEXT("%s%d|%d"),
		*SOTMPlayerStatePrivate::SpeedBoostRecordPrefix,
		bSpeedBoostUnlocked ? 1 : 0,
		SpeedBoostLevel));
	SerializedCoinIds.Add(FString::Printf(TEXT("%s%d|%d|%d|%d"),
		*SOTMPlayerStatePrivate::Phase4RecordPrefix,
		bPhase4ChestOpened ? 1 : 0,
		bPhase4HasGateKey ? 1 : 0,
		bPhase4GateUnlocked ? 1 : 0,
		bPhase4DemoCompleted ? 1 : 0));
	const bool bCollectedCoinIdsWritten =
		SOTMPlayerStatePrivate::SetStringProperty(
			SaveObject,
			SOTMPlayerStatePrivate::CollectedCoinIdsProperty,
			FString::Join(SerializedCoinIds, TEXT("\n")));

	const bool bSchemaComplete =
		bHasStateWritten &&
		bVersionWritten &&
		bLivesWritten &&
		bMaximumLivesWritten &&
		bHealthWritten &&
		bCheckpointIdWritten &&
		bCheckpointMapWritten &&
		bCheckpointTransformWritten &&
		bAvailableCoinsWritten &&
		bLifetimeCoinsWritten &&
		bCollectedCoinIdsWritten;

	if (!bSchemaComplete)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("SOTM Player State save schema mismatch: HasState=%d Version=%d Lives=%d MaxLives=%d Health=%d CheckpointId=%d CheckpointMap=%d Transform=%d AvailableCoins=%d LifetimeCoins=%d CoinIds=%d"),
			bHasStateWritten,
			bVersionWritten,
			bLivesWritten,
			bMaximumLivesWritten,
			bHealthWritten,
			bCheckpointIdWritten,
			bCheckpointMapWritten,
			bCheckpointTransformWritten,
			bAvailableCoinsWritten,
			bLifetimeCoinsWritten,
			bCollectedCoinIdsWritten);
	}

	return bSchemaComplete;
}

bool USOTMPlayerStateSubsystem::ReadStateFromSaveObject(UObject* SaveObject)
{
	if (!SaveObject)
	{
		return false;
	}

	bool bHasState = false;
	int32 SaveVersion = 0;
	if (!SOTMPlayerStatePrivate::GetBoolProperty(
			SaveObject,
			SOTMPlayerStatePrivate::HasStateProperty,
			bHasState) ||
		!bHasState ||
		!SOTMPlayerStatePrivate::GetIntProperty(
			SaveObject,
			SOTMPlayerStatePrivate::SaveVersionProperty,
			SaveVersion) ||
		SaveVersion <= 0 ||
		SaveVersion > CurrentSaveVersion)
	{
		return false;
	}

	int32 LoadedLives = CurrentLives;
	int32 LoadedMaximumLives = MaximumLives;
	float LoadedHealth = PersistentHealth;
	FName LoadedCheckpointId = NAME_None;
	FName LoadedCheckpointMap = NAME_None;
	FTransform LoadedTransform = FTransform::Identity;
	int32 LoadedAvailableCoins = 0;
	int32 LoadedLifetimeCoins = 0;
	bool bLoadedSpeedBoostUnlocked = false;
	int32 LoadedSpeedBoostLevel = 0;
	bool bLoadedPhase4ChestOpened = false;
	bool bLoadedPhase4HasGateKey = false;
	bool bLoadedPhase4GateUnlocked = false;
	bool bLoadedPhase4DemoCompleted = false;
	FString LoadedCoinIds;

	SOTMPlayerStatePrivate::GetIntProperty(SaveObject, SOTMPlayerStatePrivate::CurrentLivesProperty, LoadedLives);
	SOTMPlayerStatePrivate::GetIntProperty(SaveObject, SOTMPlayerStatePrivate::MaximumLivesProperty, LoadedMaximumLives);
	SOTMPlayerStatePrivate::GetFloatProperty(SaveObject, SOTMPlayerStatePrivate::CurrentHealthProperty, LoadedHealth);
	SOTMPlayerStatePrivate::GetNameProperty(SaveObject, SOTMPlayerStatePrivate::CheckpointIdProperty, LoadedCheckpointId);
	SOTMPlayerStatePrivate::GetNameProperty(SaveObject, SOTMPlayerStatePrivate::CheckpointMapProperty, LoadedCheckpointMap);
	SOTMPlayerStatePrivate::GetTransformProperty(
		SaveObject,
		SOTMPlayerStatePrivate::CheckpointTransformProperty,
		LoadedTransform);

	// Schema 1 saves predate Coin persistence. Missing Phase 2 fields are an
	// accepted migration path and intentionally default to zero/empty.
	if (SaveVersion >= 2)
	{
		SOTMPlayerStatePrivate::GetIntProperty(
			SaveObject,
			SOTMPlayerStatePrivate::AvailableCoinsProperty,
			LoadedAvailableCoins);
		SOTMPlayerStatePrivate::GetIntProperty(
			SaveObject,
			SOTMPlayerStatePrivate::LifetimeCoinsCollectedProperty,
			LoadedLifetimeCoins);
		SOTMPlayerStatePrivate::GetStringProperty(
			SaveObject,
			SOTMPlayerStatePrivate::CollectedCoinIdsProperty,
			LoadedCoinIds);
	}
	// Version 2 had no spending transaction. If an older Blueprint/save omitted
	// AvailableCoins but retained lifetime progress, migrate that balance once;
	// version 3 saves never repeat this credit after a purchase.
	if (SaveVersion == 2 && LoadedAvailableCoins <= 0 && LoadedLifetimeCoins > 0)
	{
		LoadedAvailableCoins = LoadedLifetimeCoins;
	}

	const USOTMPlayerSystemSettings* Settings = GetDefault<USOTMPlayerSystemSettings>();
	MaximumLives = FMath::Max(1, LoadedMaximumLives);
	CurrentLives = FMath::Clamp(LoadedLives, 0, MaximumLives);
	PersistentHealth = FMath::Clamp(LoadedHealth, 0.0f, FMath::Max(1.0f, Settings->MaximumHealth));
	AvailableCoins = FMath::Max(0, LoadedAvailableCoins);
	LifetimeCoinsCollected = FMath::Max(AvailableCoins, LoadedLifetimeCoins);
	CollectedCoinIds.Reset();
	TArray<FString> LoadedCoinIdStrings;
	LoadedCoinIds.ParseIntoArrayLines(LoadedCoinIdStrings, true);
	for (const FString& SerializedId : LoadedCoinIdStrings)
	{
		if (SaveVersion >= 3 && SerializedId.StartsWith(SOTMPlayerStatePrivate::SpeedBoostRecordPrefix))
		{
			TArray<FString> Fields;
			SerializedId.ParseIntoArray(Fields, TEXT("|"), false);
			if (Fields.Num() == 3)
			{
				bLoadedSpeedBoostUnlocked = FCString::Atoi(*Fields[1]) != 0;
				LoadedSpeedBoostLevel = FMath::Max(0, FCString::Atoi(*Fields[2]));
			}
			continue;
		}
		if (SaveVersion >= 4 && SerializedId.StartsWith(SOTMPlayerStatePrivate::Phase4RecordPrefix))
		{
			TArray<FString> Fields;
			SerializedId.ParseIntoArray(Fields, TEXT("|"), false);
			if (Fields.Num() == 5)
			{
				bLoadedPhase4ChestOpened = FCString::Atoi(*Fields[1]) != 0;
				bLoadedPhase4HasGateKey = FCString::Atoi(*Fields[2]) != 0;
				bLoadedPhase4GateUnlocked = FCString::Atoi(*Fields[3]) != 0;
				bLoadedPhase4DemoCompleted = FCString::Atoi(*Fields[4]) != 0;
			}
			continue;
		}
		FGuid CoinId;
		if (FGuid::Parse(SerializedId, CoinId) && CoinId.IsValid())
		{
			CollectedCoinIds.Add(CoinId);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SOTM Coin: ignored invalid collected ID in save: %s"), *SerializedId);
		}
	}
	// Apply ownership only after the namespaced Phase 3 record has been parsed.
	// Assigning these fields before the loop would always restore the defaults
	// (locked/level 0) even when the save contained a valid unlock record.
	bSpeedBoostUnlocked = bLoadedSpeedBoostUnlocked && LoadedSpeedBoostLevel > 0;
	SpeedBoostLevel = bSpeedBoostUnlocked ? FMath::Max(1, LoadedSpeedBoostLevel) : 0;
	bPhase4ChestOpened = bLoadedPhase4ChestOpened;
	bPhase4HasGateKey = bLoadedPhase4HasGateKey || bPhase4ChestOpened;
	bPhase4GateUnlocked = bLoadedPhase4GateUnlocked;
	bPhase4DemoCompleted = bLoadedPhase4DemoCompleted;
	bCoinStateDirty = false;

	CheckpointState.bIsValid = !LoadedCheckpointId.IsNone() && !LoadedCheckpointMap.IsNone();
	CheckpointState.CheckpointId = LoadedCheckpointId;
	CheckpointState.MapPackageName = LoadedCheckpointMap;
	CheckpointState.RespawnTransform = LoadedTransform;
	bGameOver = CurrentLives <= 0;
	OnCoinsChanged.Broadcast(AvailableCoins, LifetimeCoinsCollected);
	OnSpeedBoostOwnershipChanged.Broadcast(bSpeedBoostUnlocked, SpeedBoostLevel);
	OnPhase4ProgressChanged.Broadcast(
		bPhase4ChestOpened, bPhase4HasGateKey, bPhase4GateUnlocked, bPhase4DemoCompleted);
	return true;
}

FName USOTMPlayerStateSubsystem::NormalizeMapPackageName(const UWorld* World)
{
	if (!World)
	{
		return NAME_None;
	}

	const FString PackageName = World->GetOutermost()->GetName();
	const FString LongPath = FPackageName::GetLongPackagePath(PackageName);
	FString ShortName = FPackageName::GetShortName(PackageName);

	if (ShortName.StartsWith(TEXT("UEDPIE_")))
	{
		const int32 PrefixEnd = ShortName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 7);
		if (PrefixEnd != INDEX_NONE)
		{
			ShortName = ShortName.Mid(PrefixEnd + 1);
		}
	}

	return FName(*(LongPath / ShortName));
}
