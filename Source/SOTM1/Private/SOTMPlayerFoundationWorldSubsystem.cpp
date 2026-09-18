#include "SOTMPlayerFoundationWorldSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "ShowFlags.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING
#include "Debug/SOTMPlayerDebugPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#endif

bool USOTMPlayerFoundationWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World &&
		(World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void USOTMPlayerFoundationWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

#if !UE_BUILD_SHIPPING
	if (InWorld.WorldType == EWorldType::PIE)
	{
		UE_LOG(LogTemp, Display, TEXT("SOTM Player Debug Panel: PIE world initialized: %s"), *InWorld.GetName());
	}
#endif

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(
			this,
			&USOTMPlayerFoundationWorldSubsystem::HandleActorSpawned));

	if (UGameInstance* GameInstance = InWorld.GetGameInstance())
	{
		if (USOTMPlayerStateSubsystem* State = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>())
		{
			State->PrepareForGameplayWorld(GetNormalizedMapPackageName(&InWorld));
		}
	}

	SchedulePlayerBinding();
}

void USOTMPlayerFoundationWorldSubsystem::Deinitialize()
{
	RestorePlayerAnimation();
	if (USOTMPlayerStateSubsystem* State = BoundPlayerState.Get())
	{
		State->OnPlayerDeathStarted.RemoveDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerDeathStarted);
		State->OnPlayerRespawned.RemoveDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerRespawned);
	}
	BoundPlayerState.Reset();

#if !UE_BUILD_SHIPPING
	DestroyDebugPanel();
#endif

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerBindingTimerHandle);
		if (ActorSpawnedHandle.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		}
	}

	ActorSpawnedHandle.Reset();
	LastBoundPlayer.Reset();
	Super::Deinitialize();
}

void USOTMPlayerFoundationWorldSubsystem::HandleActorSpawned(AActor* SpawnedActor)
{
	if (SpawnedActor && SpawnedActor->IsA<APawn>())
	{
		SchedulePlayerBinding();
	}
}

void USOTMPlayerFoundationWorldSubsystem::SchedulePlayerBinding()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PlayerBindingTimerHandle,
			this,
			&USOTMPlayerFoundationWorldSubsystem::TryBindPlayer,
			0.1f,
			false);
	}
}

void USOTMPlayerFoundationWorldSubsystem::TryBindPlayer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		SchedulePlayerBinding();
		return;
	}

	NormalizeCH1RenderState(*World, PlayerController, Pawn);
	NormalizeProductionCamera(Pawn);

	USOTMPlayerVitalComponent* Vitals = Pawn->FindComponentByClass<USOTMPlayerVitalComponent>();
	if (!Vitals)
	{
		Vitals = NewObject<USOTMPlayerVitalComponent>(
			Pawn,
			USOTMPlayerVitalComponent::StaticClass(),
			TEXT("SOTM_PlayerVitals"),
			RF_Transient);
		Pawn->AddInstanceComponent(Vitals);
		Vitals->RegisterComponent();
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (USOTMPlayerStateSubsystem* State = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>())
		{
			State->BindPlayer(Pawn, Vitals);
			if (BoundPlayerState.Get() != State)
			{
				if (USOTMPlayerStateSubsystem* PreviousState = BoundPlayerState.Get())
				{
					PreviousState->OnPlayerDeathStarted.RemoveDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerDeathStarted);
					PreviousState->OnPlayerRespawned.RemoveDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerRespawned);
				}
				BoundPlayerState = State;
			}
			State->OnPlayerDeathStarted.RemoveDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerDeathStarted);
			State->OnPlayerDeathStarted.AddDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerDeathStarted);
			State->OnPlayerRespawned.RemoveDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerRespawned);
			State->OnPlayerRespawned.AddDynamic(this, &USOTMPlayerFoundationWorldSubsystem::HandlePlayerRespawned);
			if (!PlayerDeathAnimation)
			{
				PlayerDeathAnimation = LoadObject<UAnimSequence>(
					nullptr, TEXT("/Game/Mage/Animations/Anim_Mage_Death_Forward.Anim_Mage_Death_Forward"));
			}
			LastBoundPlayer = Pawn;
		}
	}

#if !UE_BUILD_SHIPPING
	if (!InitializeDebugPanel(PlayerController))
	{
		// PIE can create its pawn before its game viewport and Slate are ready.
		// Retry instead of abandoning debug-panel registration for this session.
		SchedulePlayerBinding();
	}
#endif
}

void USOTMPlayerFoundationWorldSubsystem::HandlePlayerDeathStarted(AActor* PlayerActor)
{
	if (bDeathAnimationPlaying || !PlayerActor || PlayerActor != LastBoundPlayer.Get() || !PlayerDeathAnimation)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(PlayerActor);
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh || !Mesh->GetSkeletalMeshAsset() ||
		Mesh->GetSkeletalMeshAsset()->GetSkeleton() != PlayerDeathAnimation->GetSkeleton())
	{
		UE_LOG(LogTemp, Error, TEXT("SOTM Player Death Presentation: compatible Mage mesh/animation was not available."));
		return;
	}

	SavedPlayerAnimClass = Mesh->GetAnimClass();
	DeathPresentationMesh = Mesh;
	bDeathAnimationPlaying = true;
	Mesh->PlayAnimation(PlayerDeathAnimation, false);
	UE_LOG(LogTemp, Display,
		TEXT("SOTM Player Death Presentation: Anim_Mage_Death_Forward started once (duration %.2fs)."),
		PlayerDeathAnimation->GetPlayLength());
}

void USOTMPlayerFoundationWorldSubsystem::HandlePlayerRespawned(AActor* PlayerActor)
{
	if (!PlayerActor || PlayerActor == LastBoundPlayer.Get())
	{
		RestorePlayerAnimation();
	}
}

void USOTMPlayerFoundationWorldSubsystem::RestorePlayerAnimation()
{
	if (USkeletalMeshComponent* Mesh = DeathPresentationMesh.Get())
	{
		Mesh->Stop();
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		if (SavedPlayerAnimClass)
		{
			Mesh->SetAnimInstanceClass(SavedPlayerAnimClass);
		}
		UE_LOG(LogTemp, Display, TEXT("SOTM Player Death Presentation: production Animation Blueprint restored."));
	}

	DeathPresentationMesh.Reset();
	SavedPlayerAnimClass = nullptr;
	bDeathAnimationPlaying = false;
}

void USOTMPlayerFoundationWorldSubsystem::NormalizeCH1RenderState(
	UWorld& World,
	APlayerController* PlayerController,
	APawn* Pawn)
{
	if (bCH1RenderStateNormalized ||
		!GetNormalizedMapPackageName(&World).ToString().EndsWith(TEXT("/CH1")))
	{
		return;
	}

	UGameViewportClient* GameViewport = World.GetGameViewport();
	if (!GameViewport)
	{
		return;
	}

	// A New Game reaches CH1 through Mansion with a normal gameplay viewport, while
	// Continue can load CH1 directly into a fresh Game viewport. Initialize the same
	// minimal presentation state at the common CH1 possession boundary so neither
	// route depends on a previously visited map. This runs once per CH1 world.
	GameViewport->EngineShowFlags.SetPostProcessing(true);
	GameViewport->EngineShowFlags.SetTonemapper(true);
	GameViewport->EngineShowFlags.SetEyeAdaptation(true);
	GameViewport->EngineShowFlags.SetVisualizeHDR(false);
	GameViewport->EngineShowFlags.SetVisualizeBuffer(false);
	GameViewport->EngineShowFlags.SetVisualizeNanite(false);
	GameViewport->EngineShowFlags.SetVisualizeLumen(false);
	GameViewport->EngineShowFlags.SetVisualizeSubstrate(false);
	GameViewport->EngineShowFlags.SetVisualizeGroom(false);
	GameViewport->EngineShowFlags.SetVisualizeVirtualShadowMap(false);
	GameViewport->SetViewMode(VMI_Lit);
	GameViewport->SetCurrentBufferVisualizationMode(NAME_None);
	GameViewport->SetCurrentNaniteVisualizationMode(NAME_None);
	GameViewport->SetCurrentLumenVisualizationMode(NAME_None);
	GameViewport->SetCurrentSubstrateVisualizationMode(NAME_None);
	GameViewport->SetCurrentGroomVisualizationMode(NAME_None);
	GameViewport->SetCurrentVirtualShadowMapVisualizationMode(NAME_None);
	bCH1RenderStateNormalized = true;

	UE_LOG(LogTemp, Display,
		TEXT("SOTM CH1 Render: initialized map=%s worldType=%d controller=%s pawn=%s viewTarget=%s PostProcessing=%d Tonemapper=%d EyeAdaptation=%d viewMode=Lit debugVisualizations=cleared."),
		*GetNormalizedMapPackageName(&World).ToString(),
		static_cast<int32>(World.WorldType),
		*GetNameSafe(PlayerController),
		*GetNameSafe(Pawn),
		*GetNameSafe(PlayerController ? PlayerController->GetViewTarget() : nullptr),
		GameViewport->EngineShowFlags.PostProcessing ? 1 : 0,
		GameViewport->EngineShowFlags.Tonemapper ? 1 : 0,
		GameViewport->EngineShowFlags.EyeAdaptation ? 1 : 0);
}

void USOTMPlayerFoundationWorldSubsystem::NormalizeProductionCamera(APawn* Pawn)
{
	if (!Pawn || NormalizedPlayerCamera.IsValid())
	{
		return;
	}

	USpringArmComponent* CameraBoom = nullptr;
	UCameraComponent* PlayerCamera = nullptr;
	TInlineComponentArray<USpringArmComponent*> SpringArms(Pawn);
	for (USpringArmComponent* SpringArm : SpringArms)
	{
		if (SpringArm && SpringArm->GetFName() == TEXT("CameraBoom"))
		{
			CameraBoom = SpringArm;
			break;
		}
	}
	TInlineComponentArray<UCameraComponent*> Cameras(Pawn);
	for (UCameraComponent* Camera : Cameras)
	{
		if (Camera && Camera->GetFName() == TEXT("Camera"))
		{
			PlayerCamera = Camera;
			break;
		}
	}
	if (!CameraBoom || !PlayerCamera || PlayerCamera->GetAttachParent() != CameraBoom)
	{
		return;
	}

	PlayerCamera->AttachToComponent(CameraBoom,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		USpringArmComponent::SocketName);
	// The CH1 landscape/floor blocks the inherited Camera trace from the capsule
	// origin and collapses the boom even in open forest. Disable only this camera
	// probe; Pawn/mountain collision and movement collision remain unchanged.
	CameraBoom->bDoCollisionTest = false;
	NormalizedPlayerCamera = PlayerCamera;
}

#if !UE_BUILD_SHIPPING
bool USOTMPlayerFoundationWorldSubsystem::InitializeDebugPanel(APlayerController* PlayerController)
{
	if (DebugPanelWidget.IsValid())
	{
		return true;
	}

	UWorld* World = GetWorld();
	UGameViewportClient* GameViewport = World ? World->GetGameViewport() : nullptr;
	if (!PlayerController || !World || !GameViewport || !FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Verbose, TEXT("SOTM Player Debug Panel: waiting for PIE viewport/Slate registration."));
		return false;
	}

	DebugPanelPlayerController = PlayerController;
	DebugPanelViewportClient = GameViewport;
	DebugPanelWidget = SOTMPlayerDebugPanel::CreatePanel(this);
	DebugPanelWidget->SetVisibility(EVisibility::Collapsed);
	GameViewport->AddViewportWidgetContent(DebugPanelWidget.ToSharedRef(), 20000);

	DebugInputPreprocessor = SOTMPlayerDebugPanel::CreateInputPreprocessor(this);
	FSlateApplication::Get().RegisterInputPreProcessor(DebugInputPreprocessor, 0);

	UE_LOG(LogTemp, Display, TEXT("SOTM Player Debug Panel: Debug panel registered (F10), World=%s, WorldType=%d, Viewport=%p"),
		*World->GetName(), static_cast<int32>(World->WorldType), GameViewport);
	return true;
}

void USOTMPlayerFoundationWorldSubsystem::DestroyDebugPanel()
{
	if (DebugInputPreprocessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(DebugInputPreprocessor);
	}
	DebugInputPreprocessor.Reset();

	if (DebugPanelWidget.IsValid() && DebugPanelViewportClient.IsValid())
	{
		DebugPanelViewportClient->RemoveViewportWidgetContent(DebugPanelWidget.ToSharedRef());
	}
	if (DebugPanelWidget.IsValid())
	{
		UE_LOG(LogTemp, Display, TEXT("SOTM Player Debug Panel: removed during PIE/world cleanup."));
	}
	DebugPanelWidget.Reset();
	DebugPanelPlayerController.Reset();
	DebugPanelViewportClient.Reset();
}

void USOTMPlayerFoundationWorldSubsystem::ToggleDebugPanel()
{
	if (!DebugPanelWidget.IsValid())
	{
		return;
	}

	const bool bOpening = DebugPanelWidget->GetVisibility() == EVisibility::Collapsed;
	DebugPanelWidget->SetVisibility(bOpening ? EVisibility::Visible : EVisibility::Collapsed);
	UE_LOG(LogTemp, Display, TEXT("SOTM Player Debug Panel: Panel %s"), bOpening ? TEXT("shown") : TEXT("hidden"));
	RefreshDebugPanelInputMode();
}

void USOTMPlayerFoundationWorldSubsystem::RefreshDebugPanelInputMode()
{
	APlayerController* PlayerController = DebugPanelPlayerController.Get();
	if (!PlayerController || !DebugPanelWidget.IsValid())
	{
		return;
	}

	if (DebugPanelWidget->GetVisibility() != EVisibility::Collapsed)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
		return;
	}

	USOTMPlayerStateSubsystem* State = PlayerController->GetGameInstance()
		? PlayerController->GetGameInstance()->GetSubsystem<USOTMPlayerStateSubsystem>()
		: nullptr;
	if (State && State->IsGameOver())
	{
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
	else
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(false);
	}
}
#endif

FName USOTMPlayerFoundationWorldSubsystem::GetNormalizedMapPackageName(const UWorld* World)
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
