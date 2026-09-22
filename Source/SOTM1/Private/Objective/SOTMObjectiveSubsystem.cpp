#include "Objective/SOTMObjectiveSubsystem.h"

#include "Engine/GameInstance.h"
#include "SOTMPlayerStateSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMObjective, Log, All);

const FName USOTMObjectiveSubsystem::CollectAllForestCoinsId(TEXT("CollectAllForestCoins"));
const FName USOTMObjectiveSubsystem::UnlockSpeedBoostId(TEXT("UnlockSpeedBoost"));
const FName USOTMObjectiveSubsystem::FindChestId(TEXT("FindChest"));
const FName USOTMObjectiveSubsystem::ObtainGateKeyId(TEXT("ObtainGateKey"));
const FName USOTMObjectiveSubsystem::ReachGateId(TEXT("ReachGate"));
const FName USOTMObjectiveSubsystem::DefeatIsabelId(TEXT("DefeatIsabel"));
const FName USOTMObjectiveSubsystem::DemoCompleteId(TEXT("DemoComplete"));

void USOTMObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<USOTMPlayerStateSubsystem>();

	CollectAllForestCoins.ObjectiveId = CollectAllForestCoinsId;
	CollectAllForestCoins.DisplayName = NSLOCTEXT("SOTM", "CollectAllForestCoins", "Collect All Coins");
	CollectAllForestCoins.RequiredProgress = TotalForestCoins;
	CollectAllForestCoins.State = ESOTMObjectiveState::Locked;
	auto InitializeBinaryObjective = [](FSOTMObjectiveData& Objective, const FName Id, const FText& Name)
	{
		Objective.ObjectiveId = Id;
		Objective.DisplayName = Name;
		Objective.CurrentProgress = 0;
		Objective.RequiredProgress = 1;
		Objective.State = ESOTMObjectiveState::Locked;
	};
	InitializeBinaryObjective(UnlockSpeedBoost, UnlockSpeedBoostId,
		NSLOCTEXT("SOTM", "UnlockSpeedBoost", "Unlock Speed Boost"));
	InitializeBinaryObjective(FindChest, FindChestId,
		NSLOCTEXT("SOTM", "FindChest", "Find the Chest"));
	InitializeBinaryObjective(ObtainGateKey, ObtainGateKeyId,
		NSLOCTEXT("SOTM", "ObtainGateKey", "Obtain Gate Key"));
	InitializeBinaryObjective(ReachGate, ReachGateId,
		NSLOCTEXT("SOTM", "ReachGate", "Reach the Gate"));
	InitializeBinaryObjective(DefeatIsabel, DefeatIsabelId,
		NSLOCTEXT("SOTM", "DefeatIsabel", "Defeat Isabel"));
	InitializeBinaryObjective(DemoComplete, DemoCompleteId,
		NSLOCTEXT("SOTM", "DemoCompleteObjective", "Demo Complete"));

	PlayerState = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	if (PlayerState)
	{
		PlayerState->OnCoinsChanged.RemoveDynamic(this, &ThisClass::HandleCoinsChanged);
		PlayerState->OnCoinsChanged.AddDynamic(this, &ThisClass::HandleCoinsChanged);
		PlayerState->OnSpeedBoostOwnershipChanged.AddUniqueDynamic(this, &ThisClass::HandleSpeedBoostChanged);
		PlayerState->OnPhase4ProgressChanged.AddUniqueDynamic(this, &ThisClass::HandlePhase4ProgressChanged);
	}
	RefreshFromPersistentCoinState(false);
	RefreshPhase4Objectives(false);
}

void USOTMObjectiveSubsystem::Deinitialize()
{
	if (PlayerState)
	{
		PlayerState->OnCoinsChanged.RemoveDynamic(this, &ThisClass::HandleCoinsChanged);
		PlayerState->OnSpeedBoostOwnershipChanged.RemoveDynamic(this, &ThisClass::HandleSpeedBoostChanged);
		PlayerState->OnPhase4ProgressChanged.RemoveDynamic(this, &ThisClass::HandlePhase4ProgressChanged);
	}
	PlayerState = nullptr;
	Super::Deinitialize();
}

void USOTMObjectiveSubsystem::SetForestObjectiveActive(const bool bActive)
{
	if (bForestObjectiveActive == bActive)
	{
		RefreshFromPersistentCoinState(true);
		RefreshPhase4Objectives(true);
		return;
	}

	bForestObjectiveActive = bActive;
	RefreshFromPersistentCoinState(true);
	RefreshPhase4Objectives(true);
	UE_LOG(LogSOTMObjective, Display, TEXT("Forest objective presentation %s."),
		bForestObjectiveActive ? TEXT("activated") : TEXT("deactivated"));
}

void USOTMObjectiveSubsystem::HandleCoinsChanged(
	const int32 AvailableCoins,
	const int32 LifetimeCoinsCollected)
{
	(void)AvailableCoins;
	(void)LifetimeCoinsCollected;
	RefreshFromPersistentCoinState(false);
	RefreshPhase4Objectives(false);
}

void USOTMObjectiveSubsystem::HandleSpeedBoostChanged(const bool bUnlocked, const int32 Level)
{
	(void)bUnlocked;
	(void)Level;
	RefreshPhase4Objectives(false);
}

void USOTMObjectiveSubsystem::HandlePhase4ProgressChanged(
	const bool bChestOpened,
	const bool bHasGateKey,
	const bool bGateUnlocked,
	const bool bDemoCompleted)
{
	(void)bChestOpened;
	(void)bHasGateKey;
	(void)bGateUnlocked;
	(void)bDemoCompleted;
	RefreshPhase4Objectives(false);
}

TArray<FSOTMObjectiveData> USOTMObjectiveSubsystem::GetChapterOneObjectives() const
{
	return { CollectAllForestCoins, UnlockSpeedBoost, FindChest, ObtainGateKey, ReachGate, DefeatIsabel, DemoComplete };
}

FSOTMObjectiveData USOTMObjectiveSubsystem::GetActiveChapterOneObjective() const
{
	for (const FSOTMObjectiveData& Objective : GetChapterOneObjectives())
	{
		if (Objective.State == ESOTMObjectiveState::Active)
		{
			return Objective;
		}
	}
	return DemoComplete.State == ESOTMObjectiveState::Completed ? DemoComplete : FSOTMObjectiveData();
}

ESOTMPhase4ActionResult USOTMObjectiveSubsystem::TryOpenPhase4Chest()
{
	RefreshFromPersistentCoinState(false);
	RefreshPhase4Objectives(false);
	if (!PlayerState)
	{
		return ESOTMPhase4ActionResult::InvalidState;
	}
	if (PlayerState->IsPhase4ChestOpened() || PlayerState->HasPhase4GateKey())
	{
		return ESOTMPhase4ActionResult::AlreadyCompleted;
	}
	if (CollectAllForestCoins.State != ESOTMObjectiveState::Completed)
	{
		return ESOTMPhase4ActionResult::PreviousObjectivesIncomplete;
	}
	if (!PlayerState->IsSpeedBoostUnlocked())
	{
		return ESOTMPhase4ActionResult::MissingSpeedBoost;
	}
	return PlayerState->CommitPhase4ChestOpenedAndKey()
		? ESOTMPhase4ActionResult::Success
		: ESOTMPhase4ActionResult::SaveFailed;
}

ESOTMPhase4ActionResult USOTMObjectiveSubsystem::TryUnlockPhase4Gate()
{
	RefreshFromPersistentCoinState(false);
	RefreshPhase4Objectives(false);
	if (!PlayerState)
	{
		return ESOTMPhase4ActionResult::InvalidState;
	}
	if (PlayerState->IsPhase4GateUnlocked())
	{
		return ESOTMPhase4ActionResult::AlreadyCompleted;
	}
	if (CollectAllForestCoins.State != ESOTMObjectiveState::Completed)
	{
		return ESOTMPhase4ActionResult::PreviousObjectivesIncomplete;
	}
	if (!PlayerState->IsSpeedBoostUnlocked())
	{
		return ESOTMPhase4ActionResult::MissingSpeedBoost;
	}
	if (!PlayerState->HasPhase4GateKey())
	{
		return ESOTMPhase4ActionResult::MissingGateKey;
	}
	return PlayerState->CommitPhase4GateUnlocked()
		? ESOTMPhase4ActionResult::Success
		: ESOTMPhase4ActionResult::SaveFailed;
}

ESOTMPhase4ActionResult USOTMObjectiveSubsystem::TryCompletePhase4Demo()
{
	if (!PlayerState)
	{
		return ESOTMPhase4ActionResult::InvalidState;
	}
	if (PlayerState->IsPhase4DemoCompleted())
	{
		return ESOTMPhase4ActionResult::AlreadyCompleted;
	}
	if (!PlayerState->IsPhase4GateUnlocked())
	{
		return ESOTMPhase4ActionResult::PreviousObjectivesIncomplete;
	}
	return PlayerState->CommitPhase4DemoCompleted()
		? ESOTMPhase4ActionResult::Success
		: ESOTMPhase4ActionResult::SaveFailed;
}

ESOTMPhase4ActionResult USOTMObjectiveSubsystem::TryCompleteDefeatIsabel()
{
	// Mark Defeat Isabel objective as complete
	DefeatIsabel.CurrentProgress = 1;
	DefeatIsabel.State = ESOTMObjectiveState::Completed;
	OnObjectiveChanged.Broadcast(DefeatIsabel);
	
	UE_LOG(LogSOTMObjective, Display, TEXT("Objective %s completed"), *DefeatIsabelId.ToString());
	
	// Now complete the demo
	return TryCompletePhase4Demo();
}

FText USOTMObjectiveSubsystem::GetGateRequirementFeedback() const
{
	const bool bCoins = CollectAllForestCoins.State == ESOTMObjectiveState::Completed;
	const bool bBoost = PlayerState && PlayerState->IsSpeedBoostUnlocked();
	const bool bKey = PlayerState && PlayerState->HasPhase4GateKey();
	return FText::Format(
		NSLOCTEXT("SOTM", "GateRequirements", "GATE LOCKED\n{0} Collect All Coins\n{1} Unlock Speed Boost\n{2} Gate Key"),
		bCoins ? FText::FromString(TEXT("[DONE]")) : FText::FromString(TEXT("[ ]")),
		bBoost ? FText::FromString(TEXT("[DONE]")) : FText::FromString(TEXT("[ ]")),
		bKey ? FText::FromString(TEXT("[DONE]")) : FText::FromString(TEXT("[ ]")));
}

void USOTMObjectiveSubsystem::RefreshFromPersistentCoinState(const bool bForceBroadcast)
{
	const FSOTMObjectiveData Previous = CollectAllForestCoins;
	const int32 Lifetime = PlayerState ? PlayerState->GetLifetimeCoinsCollected() : 0;
	const int32 UniqueCount = PlayerState ? PlayerState->GetCollectedCoinCount() : 0;

	// Lifetime is the currency-independent completion measure. The unique set is
	// also persistent and guards the normal one-Coin-per-GUID production route.
	const int32 AuthoritativeProgress = FMath::Max(Lifetime, UniqueCount);
	CollectAllForestCoins.CurrentProgress = FMath::Clamp(AuthoritativeProgress, 0, TotalForestCoins);
	CollectAllForestCoins.State = CollectAllForestCoins.CurrentProgress >= TotalForestCoins
		? ESOTMObjectiveState::Completed
		: (bForestObjectiveActive ? ESOTMObjectiveState::Active : ESOTMObjectiveState::Locked);

	const bool bChanged = Previous.CurrentProgress != CollectAllForestCoins.CurrentProgress ||
		Previous.State != CollectAllForestCoins.State;
	if (bChanged || bForceBroadcast)
	{
		OnObjectiveChanged.Broadcast(CollectAllForestCoins);
		UE_LOG(LogSOTMObjective, Display, TEXT("Objective %s: %d/%d state=%d lifetime=%d unique=%d"),
			*CollectAllForestCoins.ObjectiveId.ToString(),
			CollectAllForestCoins.CurrentProgress,
			CollectAllForestCoins.RequiredProgress,
			static_cast<int32>(CollectAllForestCoins.State),
			Lifetime,
			UniqueCount);
	}
}

void USOTMObjectiveSubsystem::RefreshPhase4Objectives(const bool bForceBroadcast)
{
	const FSOTMObjectiveData PreviousUnlock = UnlockSpeedBoost;
	const FSOTMObjectiveData PreviousChest = FindChest;
	const FSOTMObjectiveData PreviousKey = ObtainGateKey;
	const FSOTMObjectiveData PreviousGate = ReachGate;
	const FSOTMObjectiveData PreviousDefeat = DefeatIsabel;
	const FSOTMObjectiveData PreviousDemo = DemoComplete;

	const bool bCoinsComplete = CollectAllForestCoins.State == ESOTMObjectiveState::Completed;
	const bool bBoost = PlayerState && PlayerState->IsSpeedBoostUnlocked();
	const bool bChest = PlayerState && PlayerState->IsPhase4ChestOpened();
	const bool bKey = PlayerState && PlayerState->HasPhase4GateKey();
	const bool bGate = PlayerState && PlayerState->IsPhase4GateUnlocked();
	const bool bDemo = PlayerState && PlayerState->IsPhase4DemoCompleted();

	UnlockSpeedBoost.CurrentProgress = bBoost ? 1 : 0;
	UnlockSpeedBoost.State = bBoost ? ESOTMObjectiveState::Completed
		: (bForestObjectiveActive && bCoinsComplete ? ESOTMObjectiveState::Active : ESOTMObjectiveState::Locked);
	FindChest.CurrentProgress = bChest ? 1 : 0;
	FindChest.State = bChest ? ESOTMObjectiveState::Completed
		: (bForestObjectiveActive && bCoinsComplete && bBoost ? ESOTMObjectiveState::Active : ESOTMObjectiveState::Locked);
	ObtainGateKey.CurrentProgress = bKey ? 1 : 0;
	ObtainGateKey.State = bKey ? ESOTMObjectiveState::Completed
		: (bChest ? ESOTMObjectiveState::Active : ESOTMObjectiveState::Locked);
	ReachGate.CurrentProgress = bGate ? 1 : 0;
	ReachGate.State = bGate ? ESOTMObjectiveState::Completed
		: (bForestObjectiveActive && bCoinsComplete && bBoost && bKey ? ESOTMObjectiveState::Active : ESOTMObjectiveState::Locked);
	DefeatIsabel.CurrentProgress = bDemo ? 1 : 0;
	DefeatIsabel.State = bDemo ? ESOTMObjectiveState::Completed
		: (bGate ? ESOTMObjectiveState::Active : ESOTMObjectiveState::Locked);
	DemoComplete.CurrentProgress = bDemo ? 1 : 0;
	DemoComplete.State = bDemo ? ESOTMObjectiveState::Completed
		: (bDemo ? ESOTMObjectiveState::Active : ESOTMObjectiveState::Locked);

	BroadcastIfChanged(PreviousUnlock, UnlockSpeedBoost, bForceBroadcast);
	BroadcastIfChanged(PreviousChest, FindChest, bForceBroadcast);
	BroadcastIfChanged(PreviousKey, ObtainGateKey, bForceBroadcast);
	BroadcastIfChanged(PreviousGate, ReachGate, bForceBroadcast);
	BroadcastIfChanged(PreviousDefeat, DefeatIsabel, bForceBroadcast);
	BroadcastIfChanged(PreviousDemo, DemoComplete, bForceBroadcast);
}

void USOTMObjectiveSubsystem::BroadcastIfChanged(
	const FSOTMObjectiveData& Previous,
	const FSOTMObjectiveData& Current,
	const bool bForceBroadcast)
{
	if (bForceBroadcast || Previous.CurrentProgress != Current.CurrentProgress || Previous.State != Current.State)
	{
		OnObjectiveChanged.Broadcast(Current);
		UE_LOG(LogSOTMObjective, Display, TEXT("Objective %s state=%d progress=%d/%d"),
			*Current.ObjectiveId.ToString(), static_cast<int32>(Current.State),
			Current.CurrentProgress, Current.RequiredProgress);
	}
}
