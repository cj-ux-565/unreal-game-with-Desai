#include "Demo/SOTMDemoPhase3WorldSubsystem.h"

#include "Ability/SOTMPhase3Settings.h"
#include "Ability/SOTMTimmyUpgradeStation.h"
#include "AI/SOTMCousinAIController.h"
#include "AI/SOTMCousinCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Objective/SOTMObjectiveSubsystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/SOTMUpgradeStationWidget.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMPhase3, Log, All);

namespace SOTMPhase3Private
{
	const FName ForestMap(TEXT("/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1"));
	const TCHAR* InteractActionPath = TEXT("/Game/MenuSystemPro/Blueprints/Input/CharacterOnFoot/IA_Interact.IA_Interact");
	const TCHAR* TimmyMeshPath = TEXT("/Game/HorrorBear/Mesh/SKM_HorrorBear.SKM_HorrorBear");
	const TCHAR* StationOpenSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_StationOpen.SFX_TEMP_StationOpen");
	const TCHAR* DeniedSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_Denied.SFX_TEMP_Denied");
	const TCHAR* UpgradeSuccessSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_UpgradeSuccess.SFX_TEMP_UpgradeSuccess");
	const TCHAR* BoostSound = TEXT("/Game/SuperPowers/Powers/Speedster/SFX/Cue/WindGust_Cue.WindGust_Cue");

	FName NormalizeMapPackageName(const UWorld* World)
	{
		if (!World)
		{
			return NAME_None;
		}
		return FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
	}
}

bool USOTMDemoPhase3WorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void USOTMDemoPhase3WorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (SOTMPhase3Private::NormalizeMapPackageName(&InWorld) != SOTMPhase3Private::ForestMap)
	{
		return;
	}

	PlayerState = InWorld.GetGameInstance()
		? InWorld.GetGameInstance()->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	if (PlayerState)
	{
		PlayerState->OnSpeedBoostOwnershipChanged.AddUniqueDynamic(this, &ThisClass::HandleOwnershipChanged);
		PlayerState->OnPlayerDeathStarted.AddUniqueDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		PlayerState->OnPlayerRespawned.AddUniqueDynamic(this, &ThisClass::HandlePlayerRespawned);
		PlayerState->OnGameOver.AddUniqueDynamic(this, &ThisClass::HandleGameOver);
	}
	InWorld.GetTimerManager().SetTimer(InitializeTimer, this, &ThisClass::InitializePhase3, 0.9f, false);
}

void USOTMDemoPhase3WorldSubsystem::Deinitialize()
{
	if (ActiveBoostAudio)
	{
		ActiveBoostAudio->Stop();
		ActiveBoostAudio = nullptr;
	}
	CloseUpgradeUI();
	RestoreMovementSpeed();
	DeactivateBoostOwnedParticles();
	UnbindProductionInput();
	if (PlayerState)
	{
		PlayerState->OnSpeedBoostOwnershipChanged.RemoveDynamic(this, &ThisClass::HandleOwnershipChanged);
		PlayerState->OnPlayerDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		PlayerState->OnPlayerRespawned.RemoveDynamic(this, &ThisClass::HandlePlayerRespawned);
		PlayerState->OnGameOver.RemoveDynamic(this, &ThisClass::HandleGameOver);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (StationActor)
	{
		StationActor->Destroy();
	}
	StationActor = nullptr;
	PlayerState = nullptr;
	Super::Deinitialize();
}

void USOTMDemoPhase3WorldSubsystem::InitializePhase3()
{
	if (!PlayerState)
	{
		return;
	}
	SpawnStationAtProductionTimmy();
	BindProductionInput();
	SetRuntimeState(PlayerState->IsSpeedBoostUnlocked()
		? ESOTMSpeedBoostRuntimeState::Ready
		: ESOTMSpeedBoostRuntimeState::Locked);
	UE_LOG(LogSOTMPhase3, Display,
		TEXT("Phase 3 initialized station=%s interact=IA_Interact(E) boost=Q unlocked=%d level=%d available=%d lifetime=%d"),
		*GetNameSafe(StationActor), PlayerState->IsSpeedBoostUnlocked(), PlayerState->GetSpeedBoostLevel(),
		PlayerState->GetAvailableCoins(), PlayerState->GetLifetimeCoinsCollected());

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase3Persistence")))
	{
		BeginDevelopmentPersistenceCheck();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase3Acceptance")))
	{
		BeginDevelopmentAcceptanceRoute();
	}
#endif
}

#if !UE_BUILD_SHIPPING
void USOTMDemoPhase3WorldSubsystem::BeginDevelopmentPersistenceCheck()
{
	if (!PlayerState || !GetWorld())
	{
		return;
	}
	const bool bLoaded = PlayerState->LoadPlayerStateFromSlot(TEXT("SOTM_Phase3_Acceptance_Slot1"), false);
	SetRuntimeState(PlayerState->IsSpeedBoostUnlocked()
		? ESOTMSpeedBoostRuntimeState::Ready : ESOTMSpeedBoostRuntimeState::Locked);
	UE_LOG(LogSOTMPhase3, Display, TEXT("[Phase3Persistence] explicit relaunch load success=%d"), bLoaded);
	LogDevelopmentAcceptanceState(TEXT("RELAUNCH_CONTINUE_LOAD"));
	CaptureDevelopmentEvidence(TEXT("14_1920x1080_Continue_READY"));
	FTimerHandle ExitTimer;
	GetWorld()->GetTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			PC->ConsoleCommand(TEXT("quit"), true);
		}
	}), 3.0f, false);
}

void USOTMDemoPhase3WorldSubsystem::BeginDevelopmentAcceptanceRoute()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!World || !Player || !PlayerState || !StationActor)
	{
		UE_LOG(LogSOTMPhase3, Error, TEXT("[Phase3Acceptance] Cannot start: world/player/state/station unavailable."));
		return;
	}

	// Preview state: incomplete objective and insufficient currency. This is a
	// development-only route; production state changes still use normal APIs.
	PlayerState->SetPhase3ProgressForDebug(120, 120, false, 0);
	const FVector PreviewLocation = StationActor->GetActorLocation() + StationActor->GetActorForwardVector() * 260.0f;
	Player->SetActorLocation(PreviewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Player->SetActorRotation((StationActor->GetActorLocation() - PreviewLocation).Rotation());
	HandleStationEntered(Player);
	LogDevelopmentAcceptanceState(TEXT("PREVIEW_INCOMPLETE_OBJECTIVE"));
	CaptureDevelopmentEvidence(TEXT("01_1280x720_Timmy_Prompt"));

	FTimerHandle LockedTimer;
	World->GetTimerManager().SetTimer(LockedTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		OpenUpgradeUI();
		LogDevelopmentAcceptanceState(TEXT("LOCKED_COLLECT_ALL_FIRST"));
		CaptureDevelopmentEvidence(TEXT("02_Locked_Objective_Requirement"));
	}), 1.0f, false);

	FTimerHandle InsufficientTimer;
	World->GetTimerManager().SetTimer(InsufficientTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		CloseUpgradeUI();
		PlayerState->SetPhase3ProgressForDebug(100, 330, false, 0);
		OpenUpgradeUI();
		const ESOTMSpeedBoostPurchaseResult Rejected = TryPurchaseSpeedBoost();
		UE_LOG(LogSOTMPhase3, Display, TEXT("[Phase3Acceptance] insufficient purchase result=%d expected=%d"),
			static_cast<int32>(Rejected), static_cast<int32>(ESOTMSpeedBoostPurchaseResult::NotEnoughCoins));
		LogDevelopmentAcceptanceState(TEXT("OBJECTIVE_COMPLETE_NOT_ENOUGH_COINS"));
		CaptureDevelopmentEvidence(TEXT("03_Not_Enough_Coins"));
	}), 2.2f, false);

	FTimerHandle ReadyTimer;
	World->GetTimerManager().SetTimer(ReadyTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		CloseUpgradeUI();
		PlayerState->SetPhase3ProgressForDebug(330, 330, false, 0);
		OpenUpgradeUI();
		LogDevelopmentAcceptanceState(TEXT("PURCHASE_READY"));
		CaptureDevelopmentEvidence(TEXT("04_Purchase_Ready"));
	}), 3.4f, false);

	FTimerHandle PurchaseTimer;
	World->GetTimerManager().SetTimer(PurchaseTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		const int32 BeforeAvailable = PlayerState->GetAvailableCoins();
		const int32 BeforeLifetime = PlayerState->GetLifetimeCoinsCollected();
		const ESOTMSpeedBoostPurchaseResult First = TryPurchaseSpeedBoost();
		const ESOTMSpeedBoostPurchaseResult Duplicate = TryPurchaseSpeedBoost();
		UE_LOG(LogSOTMPhase3, Display,
			TEXT("[Phase3Acceptance] purchase first=%d duplicate=%d available %d->%d lifetime %d->%d"),
			static_cast<int32>(First), static_cast<int32>(Duplicate), BeforeAvailable,
			PlayerState->GetAvailableCoins(), BeforeLifetime, PlayerState->GetLifetimeCoinsCollected());
		LogDevelopmentAcceptanceState(TEXT("PURCHASED_OWNED"));
		CaptureDevelopmentEvidence(TEXT("05_Purchased_OWNED"));
	}), 4.6f, false);

	FTimerHandle ActiveStateTimer;
	World->GetTimerManager().SetTimer(ActiveStateTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		CloseUpgradeUI();
		const bool bFirst = TryActivateSpeedBoost();
		const bool bStacked = TryActivateSpeedBoost();
		UE_LOG(LogSOTMPhase3, Display, TEXT("[Phase3Acceptance] activation first=%d stacked=%d expected=1,0"), bFirst, bStacked);
		LogDevelopmentAcceptanceState(TEXT("BOOST_ACTIVE"));
		CaptureDevelopmentEvidence(TEXT("06_SpeedBoost_ACTIVE"));
	}), 5.8f, false);

	FTimerHandle CooldownStateTimer;
	World->GetTimerManager().SetTimer(CooldownStateTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		const bool bReactivated = TryActivateSpeedBoost();
		UE_LOG(LogSOTMPhase3, Display, TEXT("[Phase3Acceptance] cooldown reactivation=%d expected=0"), bReactivated);
		LogDevelopmentAcceptanceState(TEXT("BOOST_COOLDOWN"));
		CaptureDevelopmentEvidence(TEXT("07_SpeedBoost_COOLDOWN"));
	}), 9.2f, false);

	FTimerHandle ReadyAgainTimer;
	World->GetTimerManager().SetTimer(ReadyAgainTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("BOOST_READY_AGAIN"));
		CaptureDevelopmentEvidence(TEXT("08_SpeedBoost_READY"));
		PositionForDevelopmentCousinChase();
	}), 19.7f, false);

	FTimerHandle SlotTimer;
	World->GetTimerManager().SetTimer(SlotTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		// Two explicit test slots prove that ownership and balances are serialized
		// per slot without changing the production menu's selected slot.
		PlayerState->SetPhase3ProgressForDebug(80, 330, true, 1);
		const bool bSlot1Saved = PlayerState->SavePlayerStateToSlot(TEXT("SOTM_Phase3_Acceptance_Slot1"));
		PlayerState->ResetRuntimeStateForNewGame();
		const bool bSlot2Saved = PlayerState->SavePlayerStateToSlot(TEXT("SOTM_Phase3_Acceptance_Slot2"));
		const bool bSlot1Loaded = PlayerState->LoadPlayerStateFromSlot(TEXT("SOTM_Phase3_Acceptance_Slot1"), false);
		const bool bSlot1Unlocked = PlayerState->IsSpeedBoostUnlocked();
		const int32 Slot1Available = PlayerState->GetAvailableCoins();
		const bool bSlot2Loaded = PlayerState->LoadPlayerStateFromSlot(TEXT("SOTM_Phase3_Acceptance_Slot2"), false);
		const bool bSlot2Unlocked = PlayerState->IsSpeedBoostUnlocked();
		const int32 Slot2Available = PlayerState->GetAvailableCoins();
		PlayerState->LoadPlayerStateFromSlot(TEXT("SOTM_Phase3_Acceptance_Slot1"), false);
		ResetRuntimeAfterDeath();
		UE_LOG(LogSOTMPhase3, Display,
			TEXT("[Phase3Acceptance] slots save1=%d save2=%d load1=%d unlocked1=%d available1=%d load2=%d unlocked2=%d available2=%d"),
			bSlot1Saved, bSlot2Saved, bSlot1Loaded, bSlot1Unlocked, Slot1Available,
			bSlot2Loaded, bSlot2Unlocked, Slot2Available);
		LogDevelopmentAcceptanceState(TEXT("SLOT1_RESTORED_READY"));
		CaptureDevelopmentEvidence(TEXT("10_Slot1_Restored_READY"));
	}), 22.2f, false);

	FTimerHandle DeathTimer;
	World->GetTimerManager().SetTimer(DeathTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		AActor* Pawn = Controller ? Controller->GetPawn() : nullptr;
		const bool bActivated = TryActivateSpeedBoost();
		const float Damage = USOTMPlayerBlueprintLibrary::ApplyPlayerDamage(this, Pawn, 10000.0f, nullptr, StationActor);
		UE_LOG(LogSOTMPhase3, Display, TEXT("[Phase3Acceptance] caught-during-boost surrogate activated=%d damage=%.1f"), bActivated, Damage);
		LogDevelopmentAcceptanceState(TEXT("DEATH_DURING_BOOST"));
	}), 23.5f, false);

	FTimerHandle RespawnTimer;
	World->GetTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("POST_RESPAWN_READY"));
		CaptureDevelopmentEvidence(TEXT("12_Post_Respawn_READY"));
	}), 29.5f, false);

	FTimerHandle ExitTimer;
	World->GetTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("FINAL"));
		if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			Controller->ConsoleCommand(TEXT("quit"), true);
		}
	}), 32.0f, false);
}

void USOTMDemoPhase3WorldSubsystem::PositionForDevelopmentCousinChase()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	ASOTMCousinCharacter* Cousin = nullptr;
	for (TActorIterator<ASOTMCousinCharacter> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			Cousin = *It;
			break;
		}
	}
	if (!Player || !Cousin)
	{
		UE_LOG(LogSOTMPhase3, Warning, TEXT("[Phase3Acceptance] Natural cousin chase evidence unavailable."));
		return;
	}
	const FVector ChaseLocation = Cousin->GetActorLocation() + Cousin->GetActorForwardVector() * 700.0f;
	Player->SetActorLocation(ChaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Player->SetActorRotation((Cousin->GetActorLocation() - ChaseLocation).Rotation());
	if (ASOTMCousinAIController* AI = Cast<ASOTMCousinAIController>(Cousin->GetController()))
	{
		if (UAIPerceptionComponent* Perception = AI->GetPerceptionComponent())
		{
			Perception->RequestStimuliListenerUpdate();
		}
	}
	const bool bActivated = TryActivateSpeedBoost();
	UE_LOG(LogSOTMPhase3, Display, TEXT("[Phase3Acceptance] natural cousin chase positioned; boost activated=%d"), bActivated);
	CaptureDevelopmentEvidence(TEXT("09_Natural_Cousin_Chase_Boost"));
}

void USOTMDemoPhase3WorldSubsystem::LogDevelopmentAcceptanceState(const TCHAR* Label) const
{
	const UWorld* World = GetWorld();
	const USOTMObjectiveSubsystem* Objectives = World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<USOTMObjectiveSubsystem>() : nullptr;
	const FSOTMObjectiveData Objective = Objectives
		? Objectives->GetCollectAllForestCoinsObjective() : FSOTMObjectiveData();
	const UCharacterMovementComponent* Movement = ResolveMovementComponent();
	UE_LOG(LogSOTMPhase3, Display,
		TEXT("[Phase3Acceptance] %s available=%d lifetime=%d objective=%d/%d objectiveState=%d unlocked=%d level=%d runtime=%d speed=%.1f lives=%d dead=%d gameOver=%d"),
		Label, PlayerState ? PlayerState->GetAvailableCoins() : -1,
		PlayerState ? PlayerState->GetLifetimeCoinsCollected() : -1,
		Objective.CurrentProgress, Objective.RequiredProgress, static_cast<int32>(Objective.State),
		PlayerState && PlayerState->IsSpeedBoostUnlocked(), PlayerState ? PlayerState->GetSpeedBoostLevel() : -1,
		static_cast<int32>(RuntimeState), Movement ? Movement->MaxWalkSpeed : -1.0f,
		PlayerState ? PlayerState->GetCurrentLives() : -1, PlayerState && PlayerState->IsPlayerDead(),
		PlayerState && PlayerState->IsGameOver());
}

void USOTMDemoPhase3WorldSubsystem::CaptureDevelopmentEvidence(const FString& Label) const
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (const UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->GetViewportSize(ViewportSize);
		}
	}
	const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(),
		TEXT("Screenshots/WindowsEditor/Phase3"), Label + TEXT(".png"));
	FScreenshotRequest::RequestScreenshot(Path, true, false);
	UE_LOG(LogSOTMPhase3, Display, TEXT("[Phase3Acceptance] screenshot viewport=%.0fx%.0f path=%s"),
		ViewportSize.X, ViewportSize.Y, *Path);
}
#endif

void USOTMDemoPhase3WorldSubsystem::SpawnStationAtProductionTimmy()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!World || !PlayerPawn || StationActor)
	{
		return;
	}

	AActor* NearestTimmy = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();
	for (TActorIterator<ASkeletalMeshActor> It(World); It; ++It)
	{
		USkeletalMeshComponent* MeshComponent = It->GetSkeletalMeshComponent();
		const USkeletalMesh* Mesh = MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr;
		if (!Mesh || Mesh->GetPathName() != SOTMPhase3Private::TimmyMeshPath)
		{
			continue;
		}
		const float DistanceSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), It->GetActorLocation());
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			NearestTimmy = *It;
		}
	}
	if (!NearestTimmy)
	{
		UE_LOG(LogSOTMPhase3, Error, TEXT("Production CH1 Timmy/HorrorBear actor was not found; station not spawned."));
		return;
	}

	FRotator Facing = (PlayerPawn->GetActorLocation() - NearestTimmy->GetActorLocation()).Rotation();
	Facing.Pitch = 0.0f;
	Facing.Roll = 0.0f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	StationActor = World->SpawnActor<ASOTMTimmyUpgradeStation>(
		ASOTMTimmyUpgradeStation::StaticClass(), NearestTimmy->GetActorLocation(), Facing, Params);
	if (StationActor)
	{
		StationActor->OnPlayerEntered.AddUObject(this, &ThisClass::HandleStationEntered);
		StationActor->OnPlayerExited.AddUObject(this, &ThisClass::HandleStationExited);
		UE_LOG(LogSOTMPhase3, Display, TEXT("Timmy station attached to %s at %s distance=%.1f"),
			*NearestTimmy->GetPathName(), *NearestTimmy->GetActorLocation().ToCompactString(),
			FMath::Sqrt(NearestDistanceSq));
	}
}

void USOTMDemoPhase3WorldSubsystem::BindProductionInput()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	UEnhancedInputComponent* EnhancedInput = PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!EnhancedInput || !InputSubsystem)
	{
		if (World)
		{
			World->GetTimerManager().SetTimer(InitializeTimer, this, &ThisClass::BindProductionInput, 0.5f, false);
		}
		return;
	}
	if (BoundEnhancedInput.IsValid())
	{
		return;
	}

	InteractInputAction = LoadObject<UInputAction>(nullptr, SOTMPhase3Private::InteractActionPath);
	if (InteractInputAction)
	{
		FEnhancedInputActionEventBinding& Binding = EnhancedInput->BindAction(
			InteractInputAction, ETriggerEvent::Started, this, &ThisClass::HandleInteractInput);
		InteractBindingHandle = Binding.GetHandle();
	}

	SpeedBoostInputAction = NewObject<UInputAction>(this, TEXT("IA_SOTM_SpeedBoost"));
	SpeedBoostInputAction->ValueType = EInputActionValueType::Boolean;
	Phase3InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_SOTM_Phase3"));
	Phase3InputContext->MapKey(SpeedBoostInputAction, EKeys::Q);
	InputSubsystem->AddMappingContext(Phase3InputContext, 50);
	FEnhancedInputActionEventBinding& BoostBinding = EnhancedInput->BindAction(
		SpeedBoostInputAction, ETriggerEvent::Started, this, &ThisClass::HandleSpeedBoostInput);
	SpeedBoostBindingHandle = BoostBinding.GetHandle();
	BoundEnhancedInput = EnhancedInput;
}

void USOTMDemoPhase3WorldSubsystem::UnbindProductionInput()
{
	if (UEnhancedInputComponent* Input = BoundEnhancedInput.Get())
	{
		if (InteractBindingHandle != 0)
		{
			Input->RemoveBindingByHandle(InteractBindingHandle);
		}
		if (SpeedBoostBindingHandle != 0)
		{
			Input->RemoveBindingByHandle(SpeedBoostBindingHandle);
		}
	}
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
					LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					if (Phase3InputContext)
					{
						InputSubsystem->RemoveMappingContext(Phase3InputContext);
					}
				}
			}
		}
	}
	BoundEnhancedInput.Reset();
	InteractBindingHandle = 0;
	SpeedBoostBindingHandle = 0;
}

void USOTMDemoPhase3WorldSubsystem::HandleInteractInput()
{
	if (bPlayerInStationRange && !UpgradeWidget)
	{
		OpenUpgradeUI();
	}
}

void USOTMDemoPhase3WorldSubsystem::HandleSpeedBoostInput()
{
	TryActivateSpeedBoost();
}

void USOTMDemoPhase3WorldSubsystem::HandleStationEntered(AActor* Actor)
{
	if (!PlayerState || !PlayerState->IsBoundPlayerActor(Actor) || PlayerState->IsPlayerDead() || PlayerState->IsGameOver())
	{
		return;
	}
	bPlayerInStationRange = true;
	OnStationPromptChanged.Broadcast(true);
}

void USOTMDemoPhase3WorldSubsystem::HandleStationExited(AActor* Actor)
{
	if (!PlayerState || !PlayerState->IsBoundPlayerActor(Actor))
	{
		return;
	}
	bPlayerInStationRange = false;
	OnStationPromptChanged.Broadcast(false);
}

void USOTMDemoPhase3WorldSubsystem::OpenUpgradeUI()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!bPlayerInStationRange || UpgradeWidget || !PC || !PlayerState ||
		PlayerState->IsPlayerDead() || PlayerState->IsGameOver())
	{
		return;
	}

	UpgradeWidget = CreateWidget<USOTMUpgradeStationWidget>(PC, USOTMUpgradeStationWidget::StaticClass());
	if (!UpgradeWidget)
	{
		return;
	}
	UpgradeWidget->AddToViewport(600);
	bPreviousMouseCursor = PC->bShowMouseCursor;
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(UpgradeWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PlayerState->AcquireInputLock(ESOTMInputLockReason::Custom);
	bUpgradeInputLockHeld = true;
	OnStationPromptChanged.Broadcast(false);
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase3Private::StationOpenSound))
	{
		UGameplayStatics::PlaySound2D(this, Sound, 0.50f);
	}
	UE_LOG(LogSOTMPhase3, Display, TEXT("Timmy Upgrade Station UI opened."));
}

void USOTMDemoPhase3WorldSubsystem::CloseUpgradeUI()
{
	if (UpgradeWidget)
	{
		UpgradeWidget->RemoveFromParent();
		UpgradeWidget = nullptr;
	}
	if (PlayerState && bUpgradeInputLockHeld)
	{
		PlayerState->ReleaseInputLock(ESOTMInputLockReason::Custom);
	}
	bUpgradeInputLockHeld = false;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->bShowMouseCursor = bPreviousMouseCursor;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
	}
	if (bPlayerInStationRange)
	{
		OnStationPromptChanged.Broadcast(true);
	}
}

ESOTMSpeedBoostPurchaseResult USOTMDemoPhase3WorldSubsystem::TryPurchaseSpeedBoost()
{
	if (!PlayerState)
	{
		return ESOTMSpeedBoostPurchaseResult::NoActiveSave;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	USOTMObjectiveSubsystem* Objectives = GameInstance
		? GameInstance->GetSubsystem<USOTMObjectiveSubsystem>() : nullptr;
	const bool bObjectiveComplete = Objectives &&
		Objectives->GetCollectAllForestCoinsObjective().State == ESOTMObjectiveState::Completed;
	const ESOTMSpeedBoostPurchaseResult Result = PlayerState->TryPurchaseSpeedBoost(
		GetDefault<USOTMPhase3Settings>()->SpeedBoostUnlockCost,
		bObjectiveComplete);
	if (Result == ESOTMSpeedBoostPurchaseResult::Success)
	{
		SetRuntimeState(ESOTMSpeedBoostRuntimeState::Ready);
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase3Private::UpgradeSuccessSound))
		{
			UGameplayStatics::PlaySound2D(this, Sound, 0.62f);
		}
	}
	else if (Result != ESOTMSpeedBoostPurchaseResult::AlreadyOwned)
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase3Private::DeniedSound))
		{
			UGameplayStatics::PlaySound2D(this, Sound, 0.48f);
		}
	}
	UE_LOG(LogSOTMPhase3, Display, TEXT("Speed Boost purchase result=%d"), static_cast<int32>(Result));
	return Result;
}

bool USOTMDemoPhase3WorldSubsystem::TryActivateSpeedBoost()
{
	if (!PlayerState || RuntimeState != ESOTMSpeedBoostRuntimeState::Ready ||
		!PlayerState->IsSpeedBoostUnlocked() || PlayerState->IsPlayerDead() ||
		PlayerState->IsGameOver() || PlayerState->HasAnyInputLock())
	{
		return false;
	}
	UCharacterMovementComponent* Movement = ResolveMovementComponent();
	UWorld* World = GetWorld();
	if (!Movement || !World)
	{
		return false;
	}

	const USOTMPhase3Settings* Settings = GetDefault<USOTMPhase3Settings>();
	BoostedMovement = Movement;
	BaseSpeedBeforeBoost = FMath::Max(1.0f, Movement->MaxWalkSpeed);
	LastAppliedBoostedSpeed = BaseSpeedBeforeBoost * Settings->SpeedBoostMultiplier;
	Movement->MaxWalkSpeed = LastAppliedBoostedSpeed;
	World->GetTimerManager().SetTimer(ActiveTimer, this, &ThisClass::FinishActiveSpeedBoost,
		Settings->SpeedBoostDuration, false);
	World->GetTimerManager().SetTimer(PresentationTimer, this, &ThisClass::UpdateRuntimePresentation,
		0.1f, true);
	SetRuntimeState(ESOTMSpeedBoostRuntimeState::Active, Settings->SpeedBoostDuration);
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase3Private::BoostSound))
	{
		ActiveBoostAudio = UGameplayStatics::SpawnSound2D(
			this, Sound, 0.34f, 1.0f, 0.0f, nullptr, false, false);
	}
	// Presentation only: light up the existing player P_Rays/lightning burst.
	// Speed, duration, cooldown, audio and HUD handling above are unchanged.
	ActivateBoostOwnedParticles();
	UE_LOG(LogSOTMPhase3, Display, TEXT("Speed Boost ACTIVE base=%.1f boosted=%.1f multiplier=%.2f duration=%.1f"),
		BaseSpeedBeforeBoost, LastAppliedBoostedSpeed, Settings->SpeedBoostMultiplier,
		Settings->SpeedBoostDuration);
	return true;
}

void USOTMDemoPhase3WorldSubsystem::FinishActiveSpeedBoost()
{
	UWorld* World = GetWorld();
	if (!World || RuntimeState != ESOTMSpeedBoostRuntimeState::Active)
	{
		return;
	}
	RestoreMovementSpeed();
	// Presentation only: switch off exactly the burst components this boost lit.
	DeactivateBoostOwnedParticles();
	if (ActiveBoostAudio)
	{
		ActiveBoostAudio->FadeOut(0.18f, 0.0f);
		ActiveBoostAudio = nullptr;
	}
	const float Cooldown = GetDefault<USOTMPhase3Settings>()->SpeedBoostCooldown;
	World->GetTimerManager().SetTimer(CooldownTimer, this, &ThisClass::FinishCooldown, Cooldown, false);
	SetRuntimeState(ESOTMSpeedBoostRuntimeState::Cooldown, Cooldown);
}

void USOTMDemoPhase3WorldSubsystem::FinishCooldown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PresentationTimer);
	}
	SetRuntimeState(PlayerState && PlayerState->IsSpeedBoostUnlocked()
		? ESOTMSpeedBoostRuntimeState::Ready
		: ESOTMSpeedBoostRuntimeState::Locked);
}

void USOTMDemoPhase3WorldSubsystem::UpdateRuntimePresentation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (RuntimeState == ESOTMSpeedBoostRuntimeState::Active)
	{
		if (UCharacterMovementComponent* Movement = BoostedMovement.Get())
		{
			// If sprint or another legitimate modifier changed MaxWalkSpeed after the
			// previous application, adopt that value as the new unboosted base.
			if (!FMath::IsNearlyEqual(Movement->MaxWalkSpeed, LastAppliedBoostedSpeed, 1.0f))
			{
				BaseSpeedBeforeBoost = FMath::Max(1.0f, Movement->MaxWalkSpeed);
			}
			LastAppliedBoostedSpeed = BaseSpeedBeforeBoost *
				GetDefault<USOTMPhase3Settings>()->SpeedBoostMultiplier;
			Movement->MaxWalkSpeed = LastAppliedBoostedSpeed;
		}
		const float Remaining = FMath::Max(0.0f, World->GetTimerManager().GetTimerRemaining(ActiveTimer));
		SetRuntimeState(RuntimeState, Remaining);
	}
	else if (RuntimeState == ESOTMSpeedBoostRuntimeState::Cooldown)
	{
		const float Remaining = FMath::Max(0.0f, World->GetTimerManager().GetTimerRemaining(CooldownTimer));
		SetRuntimeState(RuntimeState, Remaining);
	}
}

void USOTMDemoPhase3WorldSubsystem::SetRuntimeState(
	const ESOTMSpeedBoostRuntimeState NewState,
	const float RemainingSeconds)
{
	RuntimeState = NewState;
	float Total = 0.0f;
	if (NewState == ESOTMSpeedBoostRuntimeState::Active)
	{
		Total = GetDefault<USOTMPhase3Settings>()->SpeedBoostDuration;
	}
	else if (NewState == ESOTMSpeedBoostRuntimeState::Cooldown)
	{
		Total = GetDefault<USOTMPhase3Settings>()->SpeedBoostCooldown;
	}
	const float Normalized = Total > 0.0f ? FMath::Clamp(RemainingSeconds / Total, 0.0f, 1.0f) : 0.0f;
	OnSpeedBoostStateChanged.Broadcast(NewState, RemainingSeconds, Normalized);
}

void USOTMDemoPhase3WorldSubsystem::RestoreMovementSpeed()
{
	if (UCharacterMovementComponent* Movement = BoostedMovement.Get())
	{
		Movement->MaxWalkSpeed = FMath::Max(1.0f, BaseSpeedBeforeBoost);
	}
	BoostedMovement.Reset();
	BaseSpeedBeforeBoost = 0.0f;
	LastAppliedBoostedSpeed = 0.0f;
}

void USOTMDemoPhase3WorldSubsystem::ActivateBoostOwnedParticles()
{
	// Never stack records: an activation always starts from a clean slate.
	DeactivateBoostOwnedParticles();
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	TArray<UParticleSystemComponent*> Components;
	Pawn->GetComponents<UParticleSystemComponent>(Components);
	for (UParticleSystemComponent* PSC : Components)
	{
		if (!IsValid(PSC) || !PSC->Template)
		{
			continue;
		}
		const FString TemplateName = PSC->Template->GetName();
		if (!TemplateName.Contains(TEXT("P_Rays")) && !TemplateName.Contains(TEXT("LightningTrail")))
		{
			continue;
		}
		// Sprint protection: components already emitting (e.g. native Shift
		// sprint) are left alone and never recorded as boost-owned.
		if (PSC->IsActive())
		{
			continue;
		}
		PSC->ActivateSystem();
		BoostOwnedParticles.Add(PSC);
	}
}

void USOTMDemoPhase3WorldSubsystem::DeactivateBoostOwnedParticles()
{
	for (TWeakObjectPtr<UParticleSystemComponent>& WeakPSC : BoostOwnedParticles)
	{
		if (UParticleSystemComponent* PSC = WeakPSC.Get())
		{
			PSC->DeactivateSystem();
		}
	}
	BoostOwnedParticles.Reset();
}

void USOTMDemoPhase3WorldSubsystem::ResetRuntimeAfterDeath()
{
	CloseUpgradeUI();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveTimer);
		World->GetTimerManager().ClearTimer(CooldownTimer);
		World->GetTimerManager().ClearTimer(PresentationTimer);
	}
	RestoreMovementSpeed();
	// A mid-boost death skips FinishActiveSpeedBoost (timer cleared), so release
	// any boost-owned burst here too. Sprint-owned components were never recorded.
	DeactivateBoostOwnedParticles();
	SetRuntimeState(PlayerState && PlayerState->IsSpeedBoostUnlocked()
		? ESOTMSpeedBoostRuntimeState::Ready
		: ESOTMSpeedBoostRuntimeState::Locked);
}

UCharacterMovementComponent* USOTMDemoPhase3WorldSubsystem::ResolveMovementComponent() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	return Character ? Character->GetCharacterMovement() : nullptr;
}

void USOTMDemoPhase3WorldSubsystem::HandleOwnershipChanged(bool bUnlocked, int32 Level)
{
	(void)Level;
	if (RuntimeState != ESOTMSpeedBoostRuntimeState::Active && RuntimeState != ESOTMSpeedBoostRuntimeState::Cooldown)
	{
		SetRuntimeState(bUnlocked ? ESOTMSpeedBoostRuntimeState::Ready : ESOTMSpeedBoostRuntimeState::Locked);
	}
}

void USOTMDemoPhase3WorldSubsystem::HandlePlayerDeathStarted(AActor* PlayerActor)
{
	(void)PlayerActor;
	bPlayerInStationRange = false;
	OnStationPromptChanged.Broadcast(false);
	ResetRuntimeAfterDeath();
}

void USOTMDemoPhase3WorldSubsystem::HandlePlayerRespawned(AActor* PlayerActor)
{
	(void)PlayerActor;
	ResetRuntimeAfterDeath();
}

void USOTMDemoPhase3WorldSubsystem::HandleGameOver()
{
	bPlayerInStationRange = false;
	OnStationPromptChanged.Broadcast(false);
	ResetRuntimeAfterDeath();
}
