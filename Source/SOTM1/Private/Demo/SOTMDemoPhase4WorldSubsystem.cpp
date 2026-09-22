#include "Demo/SOTMDemoPhase4WorldSubsystem.h"

#include "AI/SOTMIsabelAIController.h"
#include "BrainComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AI/SOTMCousinCharacter.h"
#include "Demo/SOTMDemoPhase2WorldSubsystem.h"
#include "Demo/SOTMPhase4Interactable.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Objective/SOTMObjectiveSubsystem.h"
#include "SOTMPlayerStateSubsystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/SOTMDemoCompleteWidget.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMPhase4, Log, All);

namespace SOTMPhase4Private
{
	const FName ForestMap(TEXT("/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1"));
	const TCHAR* InteractActionPath = TEXT("/Game/MenuSystemPro/Blueprints/Input/CharacterOnFoot/IA_Interact.IA_Interact");
	const TCHAR* ChestMeshPath = TEXT("/Game/Chest_Keys/chest.chest");
	const TCHAR* GateMeshPath = TEXT("/Game/Fab/Main_gate_entrance/main_gate_entrance/StaticMeshes/main_gate_entrance.main_gate_entrance");
	const TCHAR* KeyMeshPath = TEXT("/Game/Chest_Keys/GateKeys.GateKeys");
	const TCHAR* ChestOpenSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_ChestOpen.SFX_TEMP_ChestOpen");
	const TCHAR* KeyAcquiredSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_KeyAcquired.SFX_TEMP_KeyAcquired");
	const TCHAR* GateLockedSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_GateLocked.SFX_TEMP_GateLocked");
	const TCHAR* GateOpenSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_GateOpen.SFX_TEMP_GateOpen");
	const TCHAR* DemoCompleteSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_DemoComplete.SFX_TEMP_DemoComplete");

	FName NormalizeMapPackageName(const UWorld* World)
	{
		return World ? FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())) : NAME_None;
	}
}

bool USOTMDemoPhase4WorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void USOTMDemoPhase4WorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (SOTMPhase4Private::NormalizeMapPackageName(&InWorld) != SOTMPhase4Private::ForestMap)
	{
		return;
	}
	UGameInstance* GameInstance = InWorld.GetGameInstance();
	PlayerState = GameInstance ? GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	Objectives = GameInstance ? GameInstance->GetSubsystem<USOTMObjectiveSubsystem>() : nullptr;
	if (PlayerState)
	{
		PlayerState->OnPlayerDeathStarted.AddUniqueDynamic(this, &ThisClass::HandlePlayerUnavailable);
		PlayerState->OnGameOver.AddUniqueDynamic(this, &ThisClass::HideDemoComplete);
		PlayerState->OnPlayerRespawned.AddUniqueDynamic(this, &ThisClass::HandlePlayerRespawned);
	}
	InWorld.GetTimerManager().SetTimer(InitializeTimer, this, &ThisClass::InitializePhase4, 1.1f, false);
}

void USOTMDemoPhase4WorldSubsystem::Deinitialize()
{
	HideDemoComplete();
	UnbindProductionInput();
	if (PlayerState)
	{
		PlayerState->OnPlayerDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerUnavailable);
		PlayerState->OnGameOver.RemoveDynamic(this, &ThisClass::HideDemoComplete);
		PlayerState->OnPlayerRespawned.RemoveDynamic(this, &ThisClass::HandlePlayerRespawned);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (ChestAnchor) ChestAnchor->Destroy();
	if (GateAnchor) GateAnchor->Destroy();
	if (KeyPresentation) KeyPresentation->Destroy();
	ChestAnchor = nullptr;
	GateAnchor = nullptr;
	KeyPresentation = nullptr;
	ChestArt = nullptr;
	GateArt = nullptr;
	Objectives = nullptr;
	PlayerState = nullptr;
	Super::Deinitialize();
}

void USOTMDemoPhase4WorldSubsystem::InitializePhase4()
{
	if (!PlayerState || !Objectives)
	{
		UE_LOG(LogSOTMPhase4, Error, TEXT("Phase 4 initialization failed: required state subsystem missing."));
		return;
	}
	FindProductionArtAndCreateAnchors();
	BindProductionInput();
	
	// If actors not found, schedule retry
	RetryDiscoveryIfNeeded();
	
	if (PlayerState->IsPhase4ChestOpened())
	{
		BeginChestPresentation(true);
	}
	if (PlayerState->IsPhase4GateUnlocked())
	{
		BeginGatePresentation(true);
	}
	if (PlayerState->IsPhase4DemoCompleted())
	{
		GetWorld()->GetTimerManager().SetTimer(DemoCompleteTimer, this, &ThisClass::ShowDemoComplete, 0.6f, false);
	}
	UE_LOG(LogSOTMPhase4, Display,
		TEXT("Phase 4 initialized chest=%s gate=%s opened=%d key=%d gateUnlocked=%d demoComplete=%d"),
		*GetNameSafe(ChestArt), *GetNameSafe(GateArt), PlayerState->IsPhase4ChestOpened(),
		PlayerState->HasPhase4GateKey(), PlayerState->IsPhase4GateUnlocked(), PlayerState->IsPhase4DemoCompleted());

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase4DeathAfterKeyAcceptance")))
	{
		BeginDevelopmentDeathAfterKeyAcceptance();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase4Acceptance")))
	{
		BeginDevelopmentAcceptanceRoute();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase4DataAcceptance")))
	{
		RunDevelopmentDataAcceptance();
	}
#endif
}

void USOTMDemoPhase4WorldSubsystem::RetryDiscoveryIfNeeded()
{
	// Only retry if actors were not found and we haven't already initialized anchors
	if ((!ChestArt || !GateArt) && (!ChestAnchor && !GateAnchor))
	{
		UWorld* World = GetWorld();
		if (World && !World->GetTimerManager().IsTimerActive(RetryDiscoveryTimer))
		{
			UE_LOG(LogSOTMPhase4, Display, TEXT("Phase 4: scheduling retry discovery in 1.0s"));
			World->GetTimerManager().SetTimer(RetryDiscoveryTimer, this, &USOTMDemoPhase4WorldSubsystem::RetryDiscoveryTimerElapsed, 1.0f, false);
		}
	}
}

void USOTMDemoPhase4WorldSubsystem::RetryDiscoveryTimerElapsed()
{
	UE_LOG(LogSOTMPhase4, Display, TEXT("Phase 4: retrying discovery"));
	FindProductionArtAndCreateAnchors();
	
	// If still not found, try one more time after another second
	if (!ChestArt || !GateArt)
	{
		RetryDiscoveryIfNeeded();
	}
}

void USOTMDemoPhase4WorldSubsystem::FindProductionArtAndCreateAnchors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* Component = It->GetStaticMeshComponent();
		const UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
		if (Mesh && Mesh->GetPathName() == SOTMPhase4Private::ChestMeshPath)
		{
			ChestArt = *It;
			break;
		}
	}
	if (ChestArt)
	{
		ChestClosedTransform = ChestArt->GetActorTransform();
		float NearestGateSq = TNumericLimits<float>::Max();
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			UStaticMeshComponent* Component = It->GetStaticMeshComponent();
			const UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
			if (!Mesh || Mesh->GetPathName() != SOTMPhase4Private::GateMeshPath)
			{
				continue;
			}
			const float DistanceSq = FVector::DistSquared(ChestArt->GetActorLocation(), It->GetActorLocation());
			if (DistanceSq < NearestGateSq)
			{
				NearestGateSq = DistanceSq;
				GateArt = *It;
			}
		}
	}
	if (GateArt)
	{
		GateClosedTransform = GateArt->GetActorTransform();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	if (ChestArt)
	{
		ChestAnchor = World->SpawnActor<ASOTMPhase4Interactable>(
			ASOTMPhase4Interactable::StaticClass(), ChestArt->GetActorLocation(), FRotator::ZeroRotator, Params);
		if (ChestAnchor)
		{
			ChestAnchor->Configure(ESOTMPhase4InteractableKind::Chest, 425.0f);
			ChestAnchor->OnPlayerEntered.AddUObject(this, &ThisClass::HandleEntered);
			ChestAnchor->OnPlayerExited.AddUObject(this, &ThisClass::HandleExited);
		}
	}
	if (GateArt)
	{
		GateAnchor = World->SpawnActor<ASOTMPhase4Interactable>(
			ASOTMPhase4Interactable::StaticClass(), GateArt->GetActorLocation(), FRotator::ZeroRotator, Params);
		if (GateAnchor)
		{
			GateAnchor->Configure(ESOTMPhase4InteractableKind::Gate, 525.0f);
			GateAnchor->OnPlayerEntered.AddUObject(this, &ThisClass::HandleEntered);
			GateAnchor->OnPlayerExited.AddUObject(this, &ThisClass::HandleExited);
		}
	}
	if (!ChestArt || !GateArt)
	{
		UE_LOG(LogSOTMPhase4, Error, TEXT("Production Phase 4 art missing chest=%s gate=%s"),
			*GetNameSafe(ChestArt), *GetNameSafe(GateArt));
	}
}

void USOTMDemoPhase4WorldSubsystem::BindProductionInput()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	UEnhancedInputComponent* Input = PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
	if (!Input)
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
	InteractInputAction = LoadObject<UInputAction>(nullptr, SOTMPhase4Private::InteractActionPath);
	if (!InteractInputAction)
	{
		UE_LOG(LogSOTMPhase4, Error, TEXT("Production IA_Interact asset failed to load."));
		return;
	}
	FEnhancedInputActionEventBinding& Binding = Input->BindAction(
		InteractInputAction, ETriggerEvent::Started, this, &ThisClass::HandleInteractInput);
	InteractBindingHandle = Binding.GetHandle();
	BoundEnhancedInput = Input;
}

void USOTMDemoPhase4WorldSubsystem::UnbindProductionInput()
{
	if (UEnhancedInputComponent* Input = BoundEnhancedInput.Get(); Input && InteractBindingHandle != 0)
	{
		Input->RemoveBindingByHandle(InteractBindingHandle);
	}
	BoundEnhancedInput.Reset();
	InteractBindingHandle = 0;
}

void USOTMDemoPhase4WorldSubsystem::HandleInteractInput()
{
	if (!PlayerState || PlayerState->IsPlayerDead() || PlayerState->IsGameOver() || DemoCompleteWidget)
	{
		return;
	}
	if (bNearChest)
	{
		InteractWithChest();
	}
	else if (bNearGate)
	{
		InteractWithGate();
	}
}

void USOTMDemoPhase4WorldSubsystem::HandleEntered(const ESOTMPhase4InteractableKind Kind, AActor* Actor)
{
	if (!PlayerState || !PlayerState->IsBoundPlayerActor(Actor))
	{
		return;
	}
	if (Kind == ESOTMPhase4InteractableKind::Chest) bNearChest = true;
	else bNearGate = true;
	RefreshPrompt();
}

void USOTMDemoPhase4WorldSubsystem::HandleExited(const ESOTMPhase4InteractableKind Kind, AActor* Actor)
{
	if (!PlayerState || !PlayerState->IsBoundPlayerActor(Actor))
	{
		return;
	}
	if (Kind == ESOTMPhase4InteractableKind::Chest) bNearChest = false;
	else bNearGate = false;
	RefreshPrompt();
}

void USOTMDemoPhase4WorldSubsystem::RefreshPrompt()
{
	if (!PlayerState || PlayerState->IsPlayerDead() || PlayerState->IsGameOver())
	{
		OnPromptChanged.Broadcast(false, FText::GetEmpty());
		return;
	}
	if (bNearChest)
	{
		if (PlayerState->IsPhase4ChestOpened())
		{
			OnPromptChanged.Broadcast(true, NSLOCTEXT("SOTM", "ChestAlreadyOpened", "CHEST OPENED"));
		}
		else
		{
			const FSOTMObjectiveData Active = Objectives ? Objectives->GetActiveChapterOneObjective() : FSOTMObjectiveData();
			OnPromptChanged.Broadcast(true,
				Active.ObjectiveId == USOTMObjectiveSubsystem::FindChestId
					? NSLOCTEXT("SOTM", "OpenChestPrompt", "[E]  OPEN CHEST")
					: NSLOCTEXT("SOTM", "ChestLockedPrompt", "COMPLETE PREVIOUS OBJECTIVES"));
		}
		return;
	}
	if (bNearGate && !PlayerState->IsPhase4GateUnlocked())
	{
		const bool bReady = Objectives &&
			Objectives->GetActiveChapterOneObjective().ObjectiveId == USOTMObjectiveSubsystem::ReachGateId;
		OnPromptChanged.Broadcast(true, bReady
			? NSLOCTEXT("SOTM", "UnlockGatePrompt", "[E]  UNLOCK GATE")
			: (Objectives ? Objectives->GetGateRequirementFeedback() : FText::GetEmpty()));
		return;
	}
	OnPromptChanged.Broadcast(false, FText::GetEmpty());
}

void USOTMDemoPhase4WorldSubsystem::InteractWithChest()
{
	if (!Objectives || !PlayerState || PlayerState->IsPhase4ChestOpened())
	{
		return;
	}
	const ESOTMPhase4ActionResult Result = Objectives->TryOpenPhase4Chest();
	if (Result == ESOTMPhase4ActionResult::Success)
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase4Private::ChestOpenSound))
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, ChestArt ? ChestArt->GetActorLocation() : FVector::ZeroVector, 0.60f);
		}
		if (UWorld* World = GetWorld())
		{
			FTimerHandle KeySoundTimer;
			World->GetTimerManager().SetTimer(KeySoundTimer, FTimerDelegate::CreateWeakLambda(this, [this]
			{
				if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase4Private::KeyAcquiredSound))
				{
					UGameplayStatics::PlaySound2D(this, Sound, 0.58f);
				}
			}), 0.52f, false);
		}
		BeginChestPresentation(false);
		OnNotification.Broadcast(
			NSLOCTEXT("SOTM", "GateKeyAcquired", "GATE KEY ACQUIRED"),
			NSLOCTEXT("SOTM", "ReachGateUpdated", "OBJECTIVE UPDATED  -  REACH THE GATE"));
	}
	else if (Result != ESOTMPhase4ActionResult::AlreadyCompleted)
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase4Private::GateLockedSound))
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, ChestArt ? ChestArt->GetActorLocation() : FVector::ZeroVector, 0.45f);
		}
		OnNotification.Broadcast(
			NSLOCTEXT("SOTM", "ChestLocked", "CHEST LOCKED"),
			NSLOCTEXT("SOTM", "CompletePreviousObjectives", "COMPLETE PREVIOUS OBJECTIVES"));
	}
	RefreshPrompt();
	UE_LOG(LogSOTMPhase4, Display, TEXT("Chest interaction result=%d"), static_cast<int32>(Result));
}

void USOTMDemoPhase4WorldSubsystem::InteractWithGate()
{
	if (!Objectives || !PlayerState || PlayerState->IsPhase4GateUnlocked())
	{
		return;
	}
	const ESOTMPhase4ActionResult Result = Objectives->TryUnlockPhase4Gate();
	if (Result == ESOTMPhase4ActionResult::Success)
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase4Private::GateOpenSound))
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, GateArt ? GateArt->GetActorLocation() : FVector::ZeroVector, 0.62f);
		}
		OnNotification.Broadcast(
			NSLOCTEXT("SOTM", "GateUnlocked", "GATE UNLOCKED"),
			NSLOCTEXT("SOTM", "DemoGoalAchieved", "FINAL DEMO GOAL ACHIEVED"));
		BeginGatePresentation(false);
	}
	else if (Result != ESOTMPhase4ActionResult::AlreadyCompleted)
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase4Private::GateLockedSound))
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, GateArt ? GateArt->GetActorLocation() : FVector::ZeroVector, 0.48f);
		}
		OnNotification.Broadcast(
			NSLOCTEXT("SOTM", "GateLocked", "GATE LOCKED"),
			Objectives->GetGateRequirementFeedback());
	}
	RefreshPrompt();
	UE_LOG(LogSOTMPhase4, Display, TEXT("Gate interaction result=%d"), static_cast<int32>(Result));
}

void USOTMDemoPhase4WorldSubsystem::BeginChestPresentation(const bool bRestoreImmediately)
{
	if (!ChestArt || !GetWorld())
	{
		return;
	}
	ChestAnimationAlpha = bRestoreImmediately ? 1.0f : 0.0f;
	if (UStaticMeshComponent* ChestComponent = ChestArt->GetStaticMeshComponent())
	{
		ChestComponent->SetMobility(EComponentMobility::Movable);
	}
	if (!KeyPresentation)
	{
		if (UStaticMesh* KeyMesh = LoadObject<UStaticMesh>(nullptr, SOTMPhase4Private::KeyMeshPath))
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;
			KeyPresentation = GetWorld()->SpawnActor<AStaticMeshActor>(
				AStaticMeshActor::StaticClass(), ChestClosedTransform.GetLocation() + FVector(0, 0, 125),
				ChestClosedTransform.Rotator(), Params);
			if (KeyPresentation)
			{
				UStaticMeshComponent* KeyComponent = KeyPresentation->GetStaticMeshComponent();
				KeyComponent->SetMobility(EComponentMobility::Movable);
				KeyComponent->SetStaticMesh(KeyMesh);
				KeyComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				KeyPresentation->SetActorScale3D(FVector(2.2f));
			}
		}
	}
	if (bRestoreImmediately)
	{
		UpdateChestPresentation();
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
		ChestAnimationTimer, this, &ThisClass::UpdateChestPresentation, 0.03f, true);
}

void USOTMDemoPhase4WorldSubsystem::UpdateChestPresentation()
{
	if (!ChestArt || !GetWorld())
	{
		return;
	}
	ChestAnimationAlpha = FMath::Min(1.0f, ChestAnimationAlpha + 0.04f);
	const float Ease = FMath::InterpEaseOut(0.0f, 1.0f, ChestAnimationAlpha, 2.0f);
	FTransform OpenTransform = ChestClosedTransform;
	OpenTransform.SetLocation(ChestClosedTransform.GetLocation() + FVector(0, 0, 10.0f * Ease));
	FRotator Rotation = ChestClosedTransform.Rotator();
	Rotation.Roll += 5.0f * Ease;
	OpenTransform.SetRotation(Rotation.Quaternion());
	ChestArt->SetActorTransform(OpenTransform);
	if (KeyPresentation)
	{
		KeyPresentation->SetActorLocation(ChestClosedTransform.GetLocation() + FVector(0, 0, 125.0f + 65.0f * Ease));
		KeyPresentation->SetActorRotation(Rotation + FRotator(0, 120.0f * Ease, 0));
	}
	if (ChestAnimationAlpha >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(ChestAnimationTimer);
	}
}

void USOTMDemoPhase4WorldSubsystem::BeginGatePresentation(const bool bRestoreImmediately)
{
	if (!GateArt || !GetWorld())
	{
		return;
	}
	if (UStaticMeshComponent* Component = GateArt->GetStaticMeshComponent())
	{
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	bNearGate = false;
	OnPromptChanged.Broadcast(false, FText::GetEmpty());
	GateAnimationAlpha = bRestoreImmediately ? 1.0f : 0.0f;
	bGateAnimationRunning = !bRestoreImmediately;
	if (bRestoreImmediately)
	{
		UpdateGatePresentation();
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
		GateAnimationTimer, this, &ThisClass::UpdateGatePresentation, 0.03f, true);
}

void USOTMDemoPhase4WorldSubsystem::UpdateGatePresentation()
{
	if (!GateArt || !GetWorld())
	{
		return;
	}
	GateAnimationAlpha = FMath::Min(1.0f, GateAnimationAlpha + 0.025f);
	const float Ease = FMath::InterpEaseInOut(0.0f, 1.0f, GateAnimationAlpha, 2.0f);
	GateArt->SetActorLocation(GateClosedTransform.GetLocation() + FVector(0, 0, 520.0f * Ease));
	if (GateAnimationAlpha >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(GateAnimationTimer);
		bGateAnimationRunning = false;
		// Gate opened - trigger Isabel encounter instead of immediate demo complete
		if (!PlayerState || !PlayerState->IsPhase4DemoCompleted())
		{
			OnGateOpenedForIsabelEncounter.Broadcast();
			ActivateIsabelEncounter();
			UE_LOG(LogSOTMPhase4, Display, TEXT("Phase 4: Gate opened - Isabel encounter triggered"));
		}
	}
}

void USOTMDemoPhase4WorldSubsystem::FinishGatePresentation()
{
	if (Objectives)
	{
		const ESOTMPhase4ActionResult Result = Objectives->TryCompletePhase4Demo();
		if (Result != ESOTMPhase4ActionResult::Success && Result != ESOTMPhase4ActionResult::AlreadyCompleted)
		{
			UE_LOG(LogSOTMPhase4, Error, TEXT("Demo completion persistence failed result=%d"), static_cast<int32>(Result));
			return;
		}
	}
	ShowDemoComplete();
}

void USOTMDemoPhase4WorldSubsystem::ActivateIsabelEncounter()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSOTMPhase4, Error, TEXT("ActivateIsabelEncounter: No World"));
		return;
	}

	UE_LOG(LogSOTMPhase4, Display, TEXT("=== ActivateIsabelEncounter START ==="));

	// Dedicated Chapter 1 boss: the BP_IsabelAI instance (label "IsabelBoss").
	// The old generic BP_AI33 must NOT be activated anymore, so there is intentionally
	// no fallback to BP_AI33 or to any other BP_AI instance.
	ACharacter* IsabelActor = nullptr;
	FString IsabelActorName;

	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		const FString ActorLabel = It->GetActorLabel(false);
		const FString ClassName = It->GetClass()->GetName();

		if (ActorLabel == TEXT("IsabelBoss") || ClassName.Contains(TEXT("BP_IsabelAI")))
		{
			IsabelActor = *It;
			IsabelActorName = It->GetName();
			UE_LOG(LogSOTMPhase4, Display, TEXT("Dedicated Isabel boss found: Name=%s Label=%s Class=%s"),
				*IsabelActorName, *ActorLabel, *ClassName);
			break;
		}
	}

	if (!IsabelActor)
	{
		UE_LOG(LogSOTMPhase4, Error, TEXT("FAILED to find the dedicated BP_IsabelAI boss actor (label IsabelBoss)!"));
		UE_LOG(LogSOTMPhase4, Display, TEXT("=== ActivateIsabelEncounter END ==="));
		return;
	}

	UE_LOG(LogSOTMPhase4, Display, TEXT("ISABEL BOSS ACTOR FOUND: %s"), *IsabelActorName);
	UE_LOG(LogSOTMPhase4, Display, TEXT("ISABEL BOSS ACTOR CLASS: %s"), *IsabelActor->GetClass()->GetName());

	// Get the existing controller (if any)
	AAIController* ExistingController = Cast<AAIController>(IsabelActor->GetController());
	
	// Check if it already has SOTMIsabelAIController - this prevents duplicate possession
	ASOTMIsabelAIController* IsabelAI = ExistingController ? Cast<ASOTMIsabelAIController>(ExistingController) : nullptr;
	
	if (IsabelAI && IsabelAI == ExistingController)
	{
		// Already properly possessed by SOTMIsabelAIController
		UE_LOG(LogSOTMPhase4, Display, TEXT("Isabel already has SOTMIsabelAIController - initializing boss behavior"));
		
		// CRITICAL FIX: To fully disable BP_AI Blueprint's EventGraph, we need to:
		// 1. Add IsabelBoss tag
		// 2. IMPORTANT: The BP_AI Blueprint likely checks GetController() for BB in EventGraph
		//    When we replaced the controller, the Blueprint should already not have BB access
		//    But if it's still running, we need to ensure it can detect the tag EARLY
		IsabelActor->Tags.Add(TEXT("IsabelBoss"));
		UE_LOG(LogSOTMPhase4, Display, TEXT("ISABEL LEGACY BLACKBOARD DISABLED: Added IsabelBoss tag to dedicated boss"));
		
		// Additional safeguard: Clear any cached controller reference in the pawn's components
		// Force the Blueprint to re-check its controller
		IsabelActor->ForceNetUpdate();
		
		// Set boss flag
		IsabelAI->SetIsFinalBoss(true);
		
		// Explicitly initialize boss behavior: target player and enter chase
		APlayerController* PlayerController = World->GetFirstPlayerController();
		AActor* PlayerActor = PlayerController ? PlayerController->GetPawn() : nullptr;
		
		if (PlayerActor)
		{
			// Directly call the boss initialization on the controller
			IsabelAI->InitializeBossEncounter(PlayerActor);
			UE_LOG(LogSOTMPhase4, Display, TEXT("ISABEL BOSS INITIALIZED"));
		}
		else
		{
			UE_LOG(LogSOTMPhase4, Warning, TEXT("No player found for boss encounter"));
		}
		
		// Start AI brain if not already running
		if (UBrainComponent* Brain = IsabelAI->GetBrainComponent())
		{
			if (!Brain->IsRunning())
			{
				Brain->StartLogic();
				UE_LOG(LogSOTMPhase4, Display, TEXT("ISABEL MOVEMENT STARTED"));
			}
		}
		
		// Add tag to mark this as Isabel boss
		// The Isabel blueprint's EventGraph needs to check this tag and skip execution
		IsabelActor->Tags.Add(TEXT("IsabelBoss"));
		UE_LOG(LogSOTMPhase4, Display, TEXT("Added IsabelBoss tag - Isabel blueprint must check this to skip EventGraph"));
	}
	else
	{
		// Need to set up new controller
		if (ExistingController)
		{
			// Only unpossess if it's NOT already SOTMIsabelAIController
			ExistingController->UnPossess();
			ExistingController->Destroy();
			UE_LOG(LogSOTMPhase4, Display, TEXT("Removed old controller"));
		}
		
		// Spawn new SOTMIsabelAIController
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Name = TEXT("Isabel_AI_Controller");
		
		IsabelAI = World->SpawnActor<ASOTMIsabelAIController>(
			ASOTMIsabelAIController::StaticClass(), 
			IsabelActor->GetActorLocation(), 
			IsabelActor->GetActorRotation(), 
			SpawnParams);
		
		if (!IsabelAI)
		{
			UE_LOG(LogSOTMPhase4, Error, TEXT("FAILED to spawn SOTMIsabelAIController!"));
			UE_LOG(LogSOTMPhase4, Display, TEXT("=== ActivateIsabelEncounter END ==="));
			return;
		}
		
		UE_LOG(LogSOTMPhase4, Display, TEXT("Spawned SOTMIsabelAIController"));

		// Set as final boss BEFORE possession
		IsabelAI->SetIsFinalBoss(true);
		
		// Now possess - OnPossess will handle player targeting
		IsabelAI->Possess(IsabelActor);
		
		// Add tag to mark this as Isabel boss
		IsabelActor->Tags.Add(TEXT("IsabelBoss"));
		UE_LOG(LogSOTMPhase4, Display, TEXT("Added IsabelBoss tag - Isabel blueprint must check this to skip EventGraph"));
		
		UE_LOG(LogSOTMPhase4, Display, TEXT("ISABEL BOSS INITIALIZED"));
	}
	
	UE_LOG(LogSOTMPhase4, Display, TEXT("ISABEL BOSS CONTROLLER: SOTMIsabelAIController"));

	// Start the AI logic
	if (UBrainComponent* Brain = IsabelAI->GetBrainComponent())
	{
		Brain->StartLogic();
	}

	UE_LOG(LogSOTMPhase4, Display, TEXT("Isabel encounter activated successfully!"));
	UE_LOG(LogSOTMPhase4, Display, TEXT("=== ActivateIsabelEncounter END ==="));
}

void USOTMDemoPhase4WorldSubsystem::ShowDemoComplete()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC || DemoCompleteWidget)
	{
		return;
	}
	DemoCompleteWidget = CreateWidget<USOTMDemoCompleteWidget>(PC, USOTMDemoCompleteWidget::StaticClass());
	if (!DemoCompleteWidget)
	{
		return;
	}
	DemoCompleteWidget->AddToViewport(1000);
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, SOTMPhase4Private::DemoCompleteSound))
	{
		UGameplayStatics::PlaySound2D(this, Sound, 0.58f);
	}
	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	PC->SetInputMode(InputMode);
	if (PlayerState)
	{
		PlayerState->AcquireInputLock(ESOTMInputLockReason::Custom);
		bDemoInputLockHeld = true;
	}
	UE_LOG(LogSOTMPhase4, Display, TEXT("DEMO COMPLETE presentation shown; unfinished boss content not loaded."));
}

void USOTMDemoPhase4WorldSubsystem::HideDemoComplete()
{
	if (DemoCompleteWidget)
	{
		DemoCompleteWidget->RemoveFromParent();
		DemoCompleteWidget = nullptr;
	}
	if (PlayerState && bDemoInputLockHeld)
	{
		PlayerState->ReleaseInputLock(ESOTMInputLockReason::Custom);
	}
	bDemoInputLockHeld = false;
}

void USOTMDemoPhase4WorldSubsystem::HandlePlayerUnavailable(AActor* PlayerActor)
{
	(void)PlayerActor;
	bNearChest = false;
	bNearGate = false;
	OnPromptChanged.Broadcast(false, FText::GetEmpty());
}

void USOTMDemoPhase4WorldSubsystem::HandlePlayerRespawned(AActor* PlayerActor)
{
	(void)PlayerActor;
	RefreshPrompt();
}

void USOTMDemoPhase4WorldSubsystem::OnIsabelDefeated()
{
	UE_LOG(LogSOTMPhase4, Display, TEXT("Phase 4: Isabel defeated - completing demo"));
	if (Objectives)
	{
		const ESOTMPhase4ActionResult Result = Objectives->TryCompletePhase4Demo();
		if (Result != ESOTMPhase4ActionResult::Success && Result != ESOTMPhase4ActionResult::AlreadyCompleted)
		{
			UE_LOG(LogSOTMPhase4, Error, TEXT("Demo completion persistence failed result=%d"), static_cast<int32>(Result));
			return;
		}
	}
	ShowDemoComplete();
}

#if !UE_BUILD_SHIPPING
void USOTMDemoPhase4WorldSubsystem::BeginDevelopmentDeathAfterKeyAcceptance()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	USOTMDemoPhase2WorldSubsystem* Phase2 = World
		? World->GetSubsystem<USOTMDemoPhase2WorldSubsystem>() : nullptr;
	ASOTMCousinCharacter* Cousin = nullptr;
	if (World)
	{
		for (TActorIterator<ASOTMCousinCharacter> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				Cousin = *It;
				break;
			}
		}
	}
	if (!PlayerState || !Objectives || !World || !Pawn || !ChestArt || !Phase2 || !Cousin)
	{
		UE_LOG(LogSOTMPhase4, Error, TEXT("[Phase4DeathAfterKey] setup failed state=%d objectives=%d world=%d pawn=%d chest=%d phase2=%d cousin=%d"),
			PlayerState != nullptr, Objectives != nullptr, World != nullptr, Pawn != nullptr,
			ChestArt != nullptr, Phase2 != nullptr, Cousin != nullptr);
		return;
	}

	PlayerState->SetPhase3ProgressForDebug(80, 330, true, 1);
	PlayerState->SetPhase4ProgressForDebug(false, false, false, false);
	Pawn->SetActorLocation(ChestArt->GetActorLocation() + FVector(180.0f, 0.0f, 90.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	bNearChest = true;
	RefreshPrompt();
	InteractWithChest();

	const int32 LivesBeforeCatch = PlayerState->GetCurrentLives();
	const int32 AvailableBeforeCatch = PlayerState->GetAvailableCoins();
	const int32 LifetimeBeforeCatch = PlayerState->GetLifetimeCoinsCollected();
	const bool bPreconditions = PlayerState->IsPhase4ChestOpened() && PlayerState->HasPhase4GateKey() &&
		PlayerState->IsSpeedBoostUnlocked() &&
		Objectives->GetActiveChapterOneObjective().ObjectiveId == USOTMObjectiveSubsystem::ReachGateId;
	UE_LOG(LogSOTMPhase4, Display,
		TEXT("[Phase4DeathAfterKey] PRE catch preconditions=%d lives=%d coins=%d lifetime=%d chest=%d key=%d boost=%d active=%s"),
		bPreconditions, LivesBeforeCatch, AvailableBeforeCatch, LifetimeBeforeCatch,
		PlayerState->IsPhase4ChestOpened(), PlayerState->HasPhase4GateKey(),
		PlayerState->IsSpeedBoostUnlocked(), *Objectives->GetActiveChapterOneObjective().ObjectiveId.ToString());

	const FVector CatchLocation = Cousin->GetActorLocation() + Cousin->GetActorForwardVector() * 145.0f;
	Pawn->SetActorLocation(CatchLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Pawn->SetActorRotation((Cousin->GetActorLocation() - CatchLocation).Rotation());
	const bool bCatchStarted = Phase2->TryStartCousinCatch(Cousin, Pawn);
	UE_LOG(LogSOTMPhase4, Display, TEXT("[Phase4DeathAfterKey] production Cousin catch started=%d cousin=%s"),
		bCatchStarted, *GetNameSafe(Cousin));

	FTimerHandle VerifyTimer;
	World->GetTimerManager().SetTimer(VerifyTimer, FTimerDelegate::CreateWeakLambda(this,
		[this, LivesBeforeCatch, AvailableBeforeCatch, LifetimeBeforeCatch]
		{
			const bool bPass = PlayerState && Objectives && !PlayerState->IsPlayerDead() &&
				PlayerState->GetCurrentLives() == LivesBeforeCatch - 1 &&
				PlayerState->IsPhase4ChestOpened() && PlayerState->HasPhase4GateKey() &&
				PlayerState->IsSpeedBoostUnlocked() &&
				PlayerState->GetAvailableCoins() == AvailableBeforeCatch &&
				PlayerState->GetLifetimeCoinsCollected() == LifetimeBeforeCatch &&
				Objectives->GetActiveChapterOneObjective().ObjectiveId == USOTMObjectiveSubsystem::ReachGateId;
			UE_LOG(LogSOTMPhase4, Display,
				TEXT("[Phase4DeathAfterKey] PASS=%d lives=%d dead=%d chest=%d key=%d boost=%d coins=%d lifetime=%d active=%s"),
				bPass, PlayerState ? PlayerState->GetCurrentLives() : -1,
				PlayerState && PlayerState->IsPlayerDead(),
				PlayerState && PlayerState->IsPhase4ChestOpened(),
				PlayerState && PlayerState->HasPhase4GateKey(),
				PlayerState && PlayerState->IsSpeedBoostUnlocked(),
				PlayerState ? PlayerState->GetAvailableCoins() : -1,
				PlayerState ? PlayerState->GetLifetimeCoinsCollected() : -1,
				Objectives ? *Objectives->GetActiveChapterOneObjective().ObjectiveId.ToString() : TEXT("None"));
			if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
			{
				Controller->ConsoleCommand(TEXT("quit"), true);
			}
		}), 8.0f, false);
}

void USOTMDemoPhase4WorldSubsystem::BeginDevelopmentAcceptanceRoute()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerState || !Objectives || !World || !Pawn || !ChestArt || !GateArt)
	{
		UE_LOG(LogSOTMPhase4, Error, TEXT("[Phase4Acceptance] setup failed."));
		return;
	}
	PlayerState->SetPhase3ProgressForDebug(330, 330, true, 1);
	PlayerState->SetPhase4ProgressForDebug(false, false, false, false);
	Pawn->SetActorLocation(ChestArt->GetActorLocation() + FVector(180.0f, 0.0f, 90.0f), false, nullptr, ETeleportType::TeleportPhysics);
	bNearChest = true;
	RefreshPrompt();
	FScreenshotRequest::RequestScreenshot(
		FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/Phase4/01_FindChest_Prompt.png"), true, false);
	UE_LOG(LogSOTMPhase4, Display, TEXT("[Phase4Acceptance] START coins=%d lifetime=%d boost=%d active=%s"),
		PlayerState->GetAvailableCoins(), PlayerState->GetLifetimeCoinsCollected(),
		PlayerState->IsSpeedBoostUnlocked(), *Objectives->GetActiveChapterOneObjective().ObjectiveId.ToString());

	FTimerHandle OpenChestTimer;
	World->GetTimerManager().SetTimer(OpenChestTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		InteractWithChest();
		const bool bPass = PlayerState && PlayerState->IsPhase4ChestOpened() && PlayerState->HasPhase4GateKey() &&
			Objectives && Objectives->GetActiveChapterOneObjective().ObjectiveId == USOTMObjectiveSubsystem::ReachGateId;
		UE_LOG(LogSOTMPhase4, Display, TEXT("[Phase4Acceptance] CHEST pass=%d opened=%d key=%d active=%s"),
			bPass, PlayerState && PlayerState->IsPhase4ChestOpened(), PlayerState && PlayerState->HasPhase4GateKey(),
			Objectives ? *Objectives->GetActiveChapterOneObjective().ObjectiveId.ToString() : TEXT("None"));
		FScreenshotRequest::RequestScreenshot(
			FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/Phase4/02_GateKey_Acquired.png"), true, false);
	}), 0.8f, false);

	FTimerHandle ReachGateTimer;
	World->GetTimerManager().SetTimer(ReachGateTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		UWorld* CurrentWorld = GetWorld();
		APlayerController* CurrentPC = CurrentWorld ? CurrentWorld->GetFirstPlayerController() : nullptr;
		APawn* CurrentPawn = CurrentPC ? CurrentPC->GetPawn() : nullptr;
		if (!CurrentPawn || !GateArt)
		{
			return;
		}
		CurrentPawn->SetActorLocation(GateArt->GetActorLocation() + FVector(220.0f, 0.0f, 90.0f), false, nullptr, ETeleportType::TeleportPhysics);
		bNearChest = false;
		bNearGate = true;
		RefreshPrompt();
		FScreenshotRequest::RequestScreenshot(
			FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/Phase4/03_ReachGate_Prompt.png"), true, false);
		InteractWithGate();
	}), 2.0f, false);

	FTimerHandle CompleteTimer;
	World->GetTimerManager().SetTimer(CompleteTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		const bool bPass = PlayerState && PlayerState->IsPhase4GateUnlocked() && PlayerState->IsPhase4DemoCompleted() && DemoCompleteWidget;
		UE_LOG(LogSOTMPhase4, Display, TEXT("[Phase4Acceptance] COMPLETE pass=%d gate=%d demo=%d widget=%d"),
			bPass, PlayerState && PlayerState->IsPhase4GateUnlocked(),
			PlayerState && PlayerState->IsPhase4DemoCompleted(), DemoCompleteWidget != nullptr);
		FScreenshotRequest::RequestScreenshot(
			FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor/Phase4/04_DemoComplete.png"), true, false);
	}), 5.2f, false);

	FTimerHandle ReturnTimer;
	World->GetTimerManager().SetTimer(ReturnTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		UE_LOG(LogSOTMPhase4, Display, TEXT("[Phase4Acceptance] RETURN_TO_MAIN_MENU requested through production Player State route."));
		if (PlayerState)
		{
			PlayerState->ReturnToMainMenu();
		}
	}), 7.0f, false);
}

void USOTMDemoPhase4WorldSubsystem::RunDevelopmentDataAcceptance()
{
	if (!PlayerState || !Objectives)
	{
		return;
	}

	PlayerState->SetPhase3ProgressForDebug(0, 0, false, 0);
	PlayerState->SetPhase4ProgressForDebug(false, false, false, false);
	const bool bIncompleteChestRejected = Objectives->TryOpenPhase4Chest() ==
		ESOTMPhase4ActionResult::PreviousObjectivesIncomplete;
	const bool bIncompleteGateRejected = Objectives->TryUnlockPhase4Gate() ==
		ESOTMPhase4ActionResult::PreviousObjectivesIncomplete;

	PlayerState->SetPhase3ProgressForDebug(330, 330, false, 0);
	const bool bNoBoostChestRejected = Objectives->TryOpenPhase4Chest() ==
		ESOTMPhase4ActionResult::MissingSpeedBoost;
	const bool bNoBoostGateRejected = Objectives->TryUnlockPhase4Gate() ==
		ESOTMPhase4ActionResult::MissingSpeedBoost;

	PlayerState->SetPhase3ProgressForDebug(80, 330, true, 1);
	const bool bNoKeyGateRejected = Objectives->TryUnlockPhase4Gate() ==
		ESOTMPhase4ActionResult::MissingGateKey;
	const bool bChestAccepted = Objectives->TryOpenPhase4Chest() == ESOTMPhase4ActionResult::Success;
	const bool bDuplicateChestRejected = Objectives->TryOpenPhase4Chest() ==
		ESOTMPhase4ActionResult::AlreadyCompleted;
	const bool bGateAccepted = Objectives->TryUnlockPhase4Gate() == ESOTMPhase4ActionResult::Success;
	const bool bDuplicateGateRejected = Objectives->TryUnlockPhase4Gate() ==
		ESOTMPhase4ActionResult::AlreadyCompleted;
	const bool bDemoAccepted = Objectives->TryCompletePhase4Demo() == ESOTMPhase4ActionResult::Success;
	const bool bDuplicateDemoRejected = Objectives->TryCompletePhase4Demo() ==
		ESOTMPhase4ActionResult::AlreadyCompleted;
	const bool bRequirementMatrix = bIncompleteChestRejected && bIncompleteGateRejected &&
		bNoBoostChestRejected && bNoBoostGateRejected && bNoKeyGateRejected;
	const bool bOneShotFlow = bChestAccepted && bDuplicateChestRejected && bGateAccepted &&
		bDuplicateGateRejected && bDemoAccepted && bDuplicateDemoRejected;

	const FString SlotA(TEXT("SOTM_Phase4_Acceptance_SlotA"));
	const FString SlotB(TEXT("SOTM_Phase4_Acceptance_SlotB"));
	PlayerState->SetPhase3ProgressForDebug(80, 330, true, 1);
	PlayerState->SetPhase4ProgressForDebug(true, true, true, true);
	const bool bSavedA = PlayerState->SavePlayerStateToSlot(SlotA);
	PlayerState->SetPhase3ProgressForDebug(0, 0, false, 0);
	PlayerState->SetPhase4ProgressForDebug(false, false, false, false);
	const bool bSavedB = PlayerState->SavePlayerStateToSlot(SlotB);
	const bool bLoadedA = PlayerState->LoadPlayerStateFromSlot(SlotA, false);
	const bool bSlotAState = bLoadedA && PlayerState->IsPhase4ChestOpened() && PlayerState->HasPhase4GateKey() &&
		PlayerState->IsPhase4GateUnlocked() && PlayerState->IsPhase4DemoCompleted() &&
		PlayerState->IsSpeedBoostUnlocked() && PlayerState->GetLifetimeCoinsCollected() == 330 &&
		PlayerState->GetAvailableCoins() == 80;
	const bool bLoadedB = PlayerState->LoadPlayerStateFromSlot(SlotB, false);
	const bool bSlotBState = bLoadedB && !PlayerState->IsPhase4ChestOpened() && !PlayerState->HasPhase4GateKey() &&
		!PlayerState->IsPhase4GateUnlocked() && !PlayerState->IsPhase4DemoCompleted() &&
		!PlayerState->IsSpeedBoostUnlocked() && PlayerState->GetLifetimeCoinsCollected() == 0;
	PlayerState->ResetRuntimeStateForNewGame();
	const bool bNewGameReset = !PlayerState->IsPhase4ChestOpened() && !PlayerState->HasPhase4GateKey() &&
		!PlayerState->IsPhase4GateUnlocked() && !PlayerState->IsPhase4DemoCompleted();
	UE_LOG(LogSOTMPhase4, Display,
		TEXT("[Phase4DataAcceptance] PASS=%d requirements=%d oneShot=%d saveA=%d loadA=%d stateA=%d saveB=%d loadB=%d stateB=%d newGameReset=%d"),
		bRequirementMatrix && bOneShotFlow && bSavedA && bSavedB && bSlotAState && bSlotBState && bNewGameReset,
		bRequirementMatrix, bOneShotFlow, bSavedA, bLoadedA, bSlotAState, bSavedB, bLoadedB, bSlotBState, bNewGameReset);
}
#endif
