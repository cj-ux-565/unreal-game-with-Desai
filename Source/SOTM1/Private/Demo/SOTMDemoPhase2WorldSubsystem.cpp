#include "Demo/SOTMDemoPhase2WorldSubsystem.h"

#include "AI/SOTMCousinAIController.h"
#include "AI/SOTMCousinCharacter.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Coin/SOTMCoinPickup.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NavigationSystem.h"
#include "Objective/SOTMObjectiveSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "UI/SOTMIngameUIWidget.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMPhase2, Log, All);

namespace SOTMDemoPhase2Private
{
	const FName ForestMap(TEXT("/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1"));
	const TCHAR* CatchMontage = TEXT("/Game/AI/AS_CruelDoll_Attack03_Montage.AS_CruelDoll_Attack03_Montage");
	const TCHAR* CatchScream = TEXT("/Game/AI/Nightmare_scream_jumpscare_SFX.Nightmare_scream_jumpscare_SFX");
	const TCHAR* RespawnSound = TEXT("/Game/Audio/SFX/Temporary/SFX_TEMP_Respawn.SFX_TEMP_Respawn");
	const TCHAR* CousinDetectVoices[] =
	{
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_001.VO_TEMP_Cousin_Detect_001"),
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_002.VO_TEMP_Cousin_Detect_002"),
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_003.VO_TEMP_Cousin_Detect_003"),
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_004.VO_TEMP_Cousin_Detect_004")
	};
	const TCHAR* CousinDetectLines[] =
	{
		TEXT("FOUND YOU!"),
		TEXT("FRESH MEAT!"),
		TEXT("SISTER!"),
		TEXT("HE’S HERE!")
	};
	const TCHAR* CousinWhisperVoices[] =
	{
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_001.VO_TEMP_Cousin_Whisper_001"),
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_002.VO_TEMP_Cousin_Whisper_002"),
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_003.VO_TEMP_Cousin_Whisper_003"),
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_004.VO_TEMP_Cousin_Whisper_004"),
		TEXT("/Game/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_005.VO_TEMP_Cousin_Whisper_005")
	};
	const TCHAR* CousinWhisperLines[] =
	{
		TEXT("He’s back…"),
		TEXT("He can’t escape…"),
		TEXT("Sister wants him…"),
		TEXT("Let’s play chase…"),
		TEXT("RUN RUN RUN!")
	};
	constexpr int32 ProductionCousinCount = 8;
}

bool USOTMDemoPhase2WorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void USOTMDemoPhase2WorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	const bool bIsForest = GetMapPackageName(&InWorld) == SOTMDemoPhase2Private::ForestMap;
	if (USOTMObjectiveSubsystem* Objectives = InWorld.GetGameInstance()
		? InWorld.GetGameInstance()->GetSubsystem<USOTMObjectiveSubsystem>() : nullptr)
	{
		Objectives->SetForestObjectiveActive(bIsForest);
	}

	if (!bIsForest)
	{
		return;
	}

	PlayerState = InWorld.GetGameInstance()
		? InWorld.GetGameInstance()->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	if (PlayerState)
	{
		PlayerState->OnPlayerDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		PlayerState->OnPlayerDeathStarted.AddDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		PlayerState->OnPlayerRespawned.RemoveDynamic(this, &ThisClass::HandlePlayerRespawned);
		PlayerState->OnPlayerRespawned.AddDynamic(this, &ThisClass::HandlePlayerRespawned);
		PlayerState->OnGameOver.RemoveDynamic(this, &ThisClass::HandleGameOver);
		PlayerState->OnGameOver.AddDynamic(this, &ThisClass::HandleGameOver);
	}
	InWorld.GetTimerManager().SetTimer(InitializeTimer, this, &ThisClass::InitializeForestPhase2, 0.65f, false);
}

void USOTMDemoPhase2WorldSubsystem::Deinitialize()
{
	if (PlayerState)
	{
		PlayerState->OnPlayerDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		PlayerState->OnPlayerRespawned.RemoveDynamic(this, &ThisClass::HandlePlayerRespawned);
		PlayerState->OnGameOver.RemoveDynamic(this, &ThisClass::HandleGameOver);
		if (bJumpScareLockHeld)
		{
			PlayerState->ReleaseInputLock(ESOTMInputLockReason::JumpScare);
		}
	}
	bJumpScareLockHeld = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (ActiveCousinVoice)
	{
		ActiveCousinVoice->OnAudioFinished.RemoveAll(this);
		ActiveCousinVoice->Stop();
		ActiveCousinVoice = nullptr;
	}
	RemoveCousinSubtitleOverlay();
	RestorePlayerCamera();
	DestroyCatchCamera();
	if (Phase2CreatedHUD)
	{
		Phase2CreatedHUD->RemoveFromParent();
		Phase2CreatedHUD = nullptr;
	}
	SpawnedCousins.Reset();
	PlayerState = nullptr;
	Super::Deinitialize();
}

FName USOTMDemoPhase2WorldSubsystem::GetMapPackageName(const UWorld* World)
{
	return World ? FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())) : NAME_None;
}

void USOTMDemoPhase2WorldSubsystem::InitializeForestPhase2()
{
	NormalizeForestAudioMix();
	EnsureForestGameplayHUD();
	TArray<FTransform> CandidateTransforms;
	DisableLegacyForestEnemies(CandidateTransforms);
	SpawnProductionCousins(CandidateTransforms);
	UE_LOG(LogSOTMPhase2, Display,
		TEXT("Phase 2 Forest initialized: legacy normal enemies disabled=%d production cousins=%d."),
		CandidateTransforms.Num(), SpawnedCousins.Num());
	ScheduleNextCousinWhisper();

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase2Persistence")))
	{
		BeginDevelopmentPersistenceCheck();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase2Completion")))
	{
		BeginDevelopmentCompletionCheck();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase2GameOver")))
	{
		BeginDevelopmentGameOverCheck();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SOTMPhase2Acceptance")))
	{
		BeginDevelopmentAcceptanceRoute();
	}
#endif
}

void USOTMDemoPhase2WorldSubsystem::NormalizeForestAudioMix()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	int32 Adjusted = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		TInlineComponentArray<UAudioComponent*> Components(*It);
		for (UAudioComponent* Audio : Components)
		{
			USoundBase* Sound = Audio ? Audio->Sound : nullptr;
			if (!Sound)
			{
				continue;
			}
			const FString Path = Sound->GetPathName();
			float TargetVolume = -1.0f;
			if (Path.Contains(TEXT("horrorambiance3"), ESearchCase::IgnoreCase)) TargetVolume = 0.18f;
			else if (Path.Contains(TEXT("Whistling_Wind"), ESearchCase::IgnoreCase)) TargetVolume = 0.18f;
			else if (Path.Contains(TEXT("SinisterWhispers"), ESearchCase::IgnoreCase)) TargetVolume = 0.16f;
			else if (Path.Contains(TEXT("DarkDescent"), ESearchCase::IgnoreCase)) TargetVolume = 0.12f;
			else if (Path.Contains(TEXT("DreadfulLullaby"), ESearchCase::IgnoreCase)) TargetVolume = 0.10f;
			else if (Path.Contains(TEXT("Ambient_Birds_01"), ESearchCase::IgnoreCase)) TargetVolume = 0.25f;
			if (TargetVolume >= 0.0f)
			{
				Audio->SetVolumeMultiplier(TargetVolume);
				++Adjusted;
			}
		}
	}
	UE_LOG(LogSOTMPhase2, Display,
		TEXT("Temporary audio pass normalized %d existing Forest ambience/music components; no map asset was resaved."), Adjusted);
}

void USOTMDemoPhase2WorldSubsystem::ScheduleNextCousinWhisper()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CousinWhisperTimer, this, &ThisClass::PlayOccasionalCousinWhisper,
			FMath::FRandRange(12.0f, 20.0f), false);
	}
}

void USOTMDemoPhase2WorldSubsystem::PlayOccasionalCousinWhisper()
{
	if (!bCatchActive && !SpawnedCousins.IsEmpty())
	{
		const int32 CousinIndex = FMath::RandRange(0, SpawnedCousins.Num() - 1);
		const int32 VoiceIndex = FMath::RandRange(0, UE_ARRAY_COUNT(SOTMDemoPhase2Private::CousinWhisperVoices) - 1);
		if (ASOTMCousinCharacter* Cousin = SpawnedCousins[CousinIndex])
		{
			PlayTemporaryCousinVoice(
				SOTMDemoPhase2Private::CousinWhisperVoices[VoiceIndex],
				FText::FromString(SOTMDemoPhase2Private::CousinWhisperLines[VoiceIndex]),
				Cousin->GetActorLocation(), 0.24f);
		}
	}
	ScheduleNextCousinWhisper();
}

void USOTMDemoPhase2WorldSubsystem::PlayTemporaryCousinVoice(
	const TCHAR* SoundPath,
	const FText& Line,
	const FVector& Location,
	const float Volume)
{
	USoundBase* Voice = LoadObject<USoundBase>(nullptr, SoundPath);
	if (!Voice)
	{
		UE_LOG(LogSOTMPhase2, Error, TEXT("TEMPORARY PLACEHOLDER Cousin VO unavailable: %s"), SoundPath);
		return;
	}
	if (ActiveCousinVoice)
	{
		ActiveCousinVoice->OnAudioFinished.RemoveAll(this);
		ActiveCousinVoice->Stop();
		ActiveCousinVoice = nullptr;
	}
	RemoveCousinSubtitleOverlay();
	CreateCousinSubtitleOverlay();
	if (CousinSubtitleSpeakerText)
	{
		CousinSubtitleSpeakerText->SetText(NSLOCTEXT("SOTM", "CousinName", "COUSIN"));
	}
	if (CousinSubtitleLineText)
	{
		CousinSubtitleLineText->SetText(Line);
	}
	ActiveCousinVoice = UGameplayStatics::SpawnSoundAtLocation(
		this, Voice, Location, FRotator::ZeroRotator, Volume, 1.0f, 0.0f,
		nullptr, nullptr, false);
	if (!ActiveCousinVoice)
	{
		RemoveCousinSubtitleOverlay();
		return;
	}
	ActiveCousinVoice->OnAudioFinished.AddUniqueDynamic(
		this, &ThisClass::HandleTemporaryCousinVoiceFinished);
	UE_LOG(LogSOTMPhase2, Display,
		TEXT("TEMPORARY PLACEHOLDER Cousin VO: %s duration=%.2fs subtitle=%s"),
		SoundPath, Voice->GetDuration(), *Line.ToString());
}

void USOTMDemoPhase2WorldSubsystem::HandleTemporaryCousinVoiceFinished()
{
	if (ActiveCousinVoice)
	{
		ActiveCousinVoice->OnAudioFinished.RemoveAll(this);
		ActiveCousinVoice->DestroyComponent();
		ActiveCousinVoice = nullptr;
	}
	RemoveCousinSubtitleOverlay();
}

void USOTMDemoPhase2WorldSubsystem::CreateCousinSubtitleOverlay()
{
	if (CousinSubtitleRoot.IsValid())
	{
		return;
	}
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World && World->GetGameInstance()
		? World->GetGameInstance()->GetGameViewportClient() : nullptr;
	if (!Viewport)
	{
		return;
	}
	CousinSubtitleViewport = Viewport;
	TSharedRef<SWidget> Content =
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom)
		.Padding(FMargin(80.0f, 40.0f, 80.0f, 80.0f))
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.015f, 0.82f))
			.Padding(FMargin(28.0f, 16.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SAssignNew(CousinSubtitleSpeakerText, STextBlock)
					.ColorAndOpacity(FLinearColor(0.72f, 0.16f, 0.88f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				.Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SAssignNew(CousinSubtitleLineText, STextBlock)
					.ColorAndOpacity(FLinearColor::White)
					.Justification(ETextJustify::Center)
				]
			]
		];
	CousinSubtitleRoot = Content;
	Viewport->AddViewportWidgetContent(CousinSubtitleRoot.ToSharedRef(), 950);
}

void USOTMDemoPhase2WorldSubsystem::RemoveCousinSubtitleOverlay()
{
	if (CousinSubtitleRoot.IsValid())
	{
		if (UGameViewportClient* Viewport = CousinSubtitleViewport.Get())
		{
			Viewport->RemoveViewportWidgetContent(CousinSubtitleRoot.ToSharedRef());
		}
	}
	CousinSubtitleRoot.Reset();
	CousinSubtitleSpeakerText.Reset();
	CousinSubtitleLineText.Reset();
	CousinSubtitleViewport.Reset();
}

void USOTMDemoPhase2WorldSubsystem::EnsureForestGameplayHUD()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TObjectIterator<USOTMIngameUIWidget> It; It; ++It)
	{
		if (It->GetWorld() == World && It->IsInViewport())
		{
			return;
		}
	}

	APlayerController* PC = World->GetFirstPlayerController();
	UClass* HUDClass = LoadClass<USOTMIngameUIWidget>(nullptr,
		TEXT("/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI.WBP_IngameUI_C"));
	if (!PC || !HUDClass)
	{
		UE_LOG(LogSOTMPhase2, Error, TEXT("Phase 2 could not locate the production gameplay HUD/player controller."));
		return;
	}
	Phase2CreatedHUD = CreateWidget<USOTMIngameUIWidget>(PC, HUDClass);
	if (Phase2CreatedHUD)
	{
		Phase2CreatedHUD->AddToViewport(50);
		UE_LOG(LogSOTMPhase2, Display,
			TEXT("Phase 2 created the existing production WBP_IngameUI because no viewport instance existed."));
	}
}

#if !UE_BUILD_SHIPPING
void USOTMDemoPhase2WorldSubsystem::BeginDevelopmentPersistenceCheck()
{
	const bool bLoaded = PlayerState && PlayerState->LoadPlayerStateFromSlot(TEXT("Slot 1"), false);
	UE_LOG(LogSOTMPhase2, Display, TEXT("[Phase2Persistence] Load Slot 1 success=%d"), bLoaded);
	LogDevelopmentAcceptanceState(TEXT("RELAUNCH_LOAD"));
	CaptureDevelopmentEvidence(TEXT("05_Persistence_Load"));
	FTimerHandle ExitTimer;
	GetWorld()->GetTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			PC->ConsoleCommand(TEXT("quit"), true);
		}
	}), 3.0f, false);
}

void USOTMDemoPhase2WorldSubsystem::BeginDevelopmentCompletionCheck()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AActor* Player = PC ? PC->GetPawn() : nullptr;
	if (!World || !Player || !PlayerState)
	{
		return;
	}
	PlayerState->LoadPlayerStateFromSlot(TEXT("Slot 1"), false);

	TArray<ASOTMCoinPickup*> RemainingCoins;
	for (TActorIterator<ASOTMCoinPickup> It(World); It; ++It)
	{
		if (IsValid(*It) && !PlayerState->IsCoinCollected(It->GetPersistentCoinId()))
		{
			RemainingCoins.Add(*It);
		}
	}
	RemainingCoins.Sort([](const ASOTMCoinPickup& Left, const ASOTMCoinPickup& Right)
	{
		return Left.GetPathName() < Right.GetPathName();
	});
	const int32 DirectTransactionsNeeded = FMath::Max(0,
		USOTMObjectiveSubsystem::TotalForestCoins - 1 - PlayerState->GetLifetimeCoinsCollected());
	for (int32 Index = 0; Index < DirectTransactionsNeeded && Index < RemainingCoins.Num() - 1; ++Index)
	{
		PlayerState->TryCollectCoin(RemainingCoins[Index]->GetPersistentCoinId(), Player, 1);
	}
	LogDevelopmentAcceptanceState(TEXT("COMPLETION_BEFORE_FINAL_NATURAL_PICKUP"));

	ASOTMCoinPickup* FinalCoin = RemainingCoins.IsEmpty() ? nullptr : RemainingCoins.Last();
	if (IsValid(FinalCoin))
	{
		FinalCoin->NotifyActorBeginOverlap(Player);
	}
	LogDevelopmentAcceptanceState(TEXT("COMPLETION_AFTER_FINAL_NATURAL_PICKUP"));
	CaptureDevelopmentEvidence(TEXT("06_Objective_Complete"));
	FTimerHandle ExitTimer;
	World->GetTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			Controller->ConsoleCommand(TEXT("quit"), true);
		}
	}), 3.0f, false);
}

void USOTMDemoPhase2WorldSubsystem::BeginDevelopmentGameOverCheck()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AActor* Player = PC ? PC->GetPawn() : nullptr;
	if (!World || !Player || !PlayerState || SpawnedCousins.IsEmpty())
	{
		return;
	}
	PlayerState->SetLivesForDebug(1);
	FTimerHandle CatchTimer;
	World->GetTimerManager().SetTimer(CatchTimer, FTimerDelegate::CreateWeakLambda(this, [this, Player]
	{
		const bool bFirstAccepted = TryStartCousinCatch(SpawnedCousins[0], Player);
		const bool bDuplicateAccepted = SpawnedCousins.Num() > 1
			? TryStartCousinCatch(SpawnedCousins[1], Player) : false;
		UE_LOG(LogSOTMPhase2, Display,
			TEXT("[Phase2GameOver] firstCatch=%d duplicateCatch=%d (expected 1,0)"),
			bFirstAccepted, bDuplicateAccepted);
	}), 3.0f, false);
	FTimerHandle GameOverTimer;
	World->GetTimerManager().SetTimer(GameOverTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("FINAL_LIFE_GAME_OVER"));
		CaptureDevelopmentEvidence(TEXT("07_Game_Over"));
	}), 6.5f, false);
	FTimerHandle RetryTimer;
	World->GetTimerManager().SetTimer(RetryTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		const bool bRetry = PlayerState && PlayerState->RetryFromGameOver();
		UE_LOG(LogSOTMPhase2, Display, TEXT("[Phase2GameOver] RetryFromGameOver=%d"), bRetry);
	}), 7.5f, false);
	FTimerHandle FinalTimer;
	World->GetTimerManager().SetTimer(FinalTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("AFTER_GAME_OVER_RETRY"));
		CaptureDevelopmentEvidence(TEXT("08_After_Retry"));
		if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			Controller->ConsoleCommand(TEXT("quit"), true);
		}
	}), 10.0f, false);
}

void USOTMDemoPhase2WorldSubsystem::BeginDevelopmentAcceptanceRoute()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AActor* Player = PC ? PC->GetPawn() : nullptr;
	if (!World || !Player || !PlayerState)
	{
		UE_LOG(LogSOTMPhase2, Error, TEXT("[Phase2Acceptance] Cannot start: world/player/state unavailable."));
		return;
	}

	DevelopmentCoinRoute.Reset();
	for (TActorIterator<ASOTMCoinPickup> It(World); It; ++It)
	{
		ASOTMCoinPickup* Coin = *It;
		if (IsValid(Coin) && !PlayerState->IsCoinCollected(Coin->GetPersistentCoinId()))
		{
			DevelopmentCoinRoute.Add(Coin);
		}
	}
	DevelopmentCoinRoute.Sort([Player](const TWeakObjectPtr<ASOTMCoinPickup>& Left,
		const TWeakObjectPtr<ASOTMCoinPickup>& Right)
	{
		return FVector::DistSquared(Player->GetActorLocation(), Left->GetActorLocation()) <
			FVector::DistSquared(Player->GetActorLocation(), Right->GetActorLocation());
	});
	if (DevelopmentCoinRoute.Num() > 5)
	{
		DevelopmentCoinRoute.SetNum(5);
	}
	DevelopmentCoinRouteIndex = 0;
	bDevelopmentAcceptanceCatchObserved = false;

	UE_LOG(LogSOTMPhase2, Display,
		TEXT("[Phase2Acceptance] START map=%s naturalCoinRoute=%d cousins=%d"),
		*GetMapPackageName(World).ToString(), DevelopmentCoinRoute.Num(), SpawnedCousins.Num());
	LogDevelopmentAcceptanceState(TEXT("CH1_START"));
	CaptureDevelopmentEvidence(TEXT("01_CH1_Start"));

	World->GetTimerManager().SetTimer(
		DevelopmentCoinTimer, this, &ThisClass::CollectNextDevelopmentCoin, 0.65f, true, 1.0f);
	FTimerHandle EncounterTimer;
	World->GetTimerManager().SetTimer(
		EncounterTimer, this, &ThisClass::PositionCousinForDevelopmentEncounter, 5.5f, false);
	FTimerHandle PostCoinsTimer;
	World->GetTimerManager().SetTimer(PostCoinsTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("AFTER_FIVE_NATURAL_COINS"));
		CaptureDevelopmentEvidence(TEXT("02_Five_Coins"));
	}), 4.8f, false);
	FTimerHandle PostCatchTimer;
	World->GetTimerManager().SetTimer(PostCatchTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("POST_CATCH_WINDOW"));
		CaptureDevelopmentEvidence(TEXT("03_Post_Catch"));
	}), 12.0f, false);
	FTimerHandle PostRespawnTimer;
	World->GetTimerManager().SetTimer(PostRespawnTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("POST_RESPAWN_WINDOW"));
		CaptureDevelopmentEvidence(TEXT("04_Post_Respawn"));
	}), 17.0f, false);
	FTimerHandle ExitTimer;
	World->GetTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		LogDevelopmentAcceptanceState(TEXT("FINAL"));
		if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			Controller->ConsoleCommand(TEXT("quit"), true);
		}
	}), 21.0f, false);
}

void USOTMDemoPhase2WorldSubsystem::CollectNextDevelopmentCoin()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AActor* Player = PC ? PC->GetPawn() : nullptr;
	if (!World || !Player || !DevelopmentCoinRoute.IsValidIndex(DevelopmentCoinRouteIndex))
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(DevelopmentCoinTimer);
		}
		return;
	}

	ASOTMCoinPickup* Coin = DevelopmentCoinRoute[DevelopmentCoinRouteIndex++].Get();
	if (IsValid(Coin))
	{
		// Exercise the production pickup actor's overlap path; do not inject Coin state.
		Coin->NotifyActorBeginOverlap(Player);
		UE_LOG(LogSOTMPhase2, Display, TEXT("[Phase2Acceptance] Natural pickup %d/5 via %s"),
			DevelopmentCoinRouteIndex, *GetNameSafe(Coin));
	}
	if (DevelopmentCoinRouteIndex >= DevelopmentCoinRoute.Num())
	{
		World->GetTimerManager().ClearTimer(DevelopmentCoinTimer);
	}
}

void USOTMDemoPhase2WorldSubsystem::PositionCousinForDevelopmentEncounter()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	ASOTMCousinCharacter* Cousin = SpawnedCousins.IsEmpty() ? nullptr : SpawnedCousins[0];
	if (!World || !Player || !IsValid(Cousin))
	{
		UE_LOG(LogSOTMPhase2, Error, TEXT("[Phase2Acceptance] Cousin encounter unavailable."));
		return;
	}

	FVector Destination = Cousin->GetActorLocation() + Cousin->GetActorForwardVector() * 525.0f;
	if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation Projected;
		if (Navigation->ProjectPointToNavigation(Destination, Projected, FVector(300.0f)))
		{
			Destination = Projected.Location;
		}
	}
	Player->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
	Player->SetActorRotation((Cousin->GetActorLocation() - Destination).Rotation());
	if (ASOTMCousinAIController* CousinController =
		Cast<ASOTMCousinAIController>(Cousin->GetController()))
	{
		if (UAIPerceptionComponent* Perception = CousinController->GetPerceptionComponent())
		{
			Perception->RequestStimuliListenerUpdate();
		}
	}
	UE_LOG(LogSOTMPhase2, Display,
		TEXT("[Phase2Acceptance] Player positioned near an existing patrol cousin on valid Forest navigation; detection remains perception driven."));
	CaptureDevelopmentEvidence(TEXT("03_Cousin_Encounter"));

	TWeakObjectPtr<ASOTMCousinCharacter> WeakCousin(Cousin);
	TWeakObjectPtr<APawn> WeakPlayer(Player);
	FTimerHandle CatchRangeTimer;
	World->GetTimerManager().SetTimer(CatchRangeTimer, FTimerDelegate::CreateWeakLambda(this,
		[this, WeakCousin, WeakPlayer]
		{
			ASOTMCousinCharacter* ActiveCousin = WeakCousin.Get();
			APawn* ActivePlayer = WeakPlayer.Get();
			if (!IsValid(ActiveCousin) || !IsValid(ActivePlayer) || bCatchActive ||
				bDevelopmentAcceptanceCatchObserved)
			{
				return;
			}
			const FVector CatchLocation = ActiveCousin->GetActorLocation() +
				ActiveCousin->GetActorForwardVector() * 150.0f;
			ActivePlayer->SetActorLocation(CatchLocation, false, nullptr, ETeleportType::TeleportPhysics);
			ActivePlayer->SetActorRotation((ActiveCousin->GetActorLocation() - CatchLocation).Rotation());
			if (ASOTMCousinAIController* CousinController =
				Cast<ASOTMCousinAIController>(ActiveCousin->GetController()))
			{
				if (UAIPerceptionComponent* Perception = CousinController->GetPerceptionComponent())
				{
					Perception->RequestStimuliListenerUpdate();
				}
			}
			UE_LOG(LogSOTMPhase2, Display,
				TEXT("[Phase2Acceptance] Player moved inside the detected cousin's catch range; catch itself not invoked directly."));
			FTimerHandle CatchFallbackTimer;
			GetWorld()->GetTimerManager().SetTimer(CatchFallbackTimer,
				FTimerDelegate::CreateWeakLambda(this, [this, WeakCousin, WeakPlayer]
				{
					if (!bCatchActive && !bDevelopmentAcceptanceCatchObserved &&
						WeakCousin.IsValid() && WeakPlayer.IsValid())
					{
						UE_LOG(LogSOTMPhase2, Warning,
							TEXT("[Phase2Acceptance] Perception/nav did not begin catch in the timed window; invoking the public catch gate to verify cinematic/death integration."));
						TryStartCousinCatch(WeakCousin.Get(), WeakPlayer.Get());
					}
				}), 1.0f, false);
		}), 4.0f, false);
}

void USOTMDemoPhase2WorldSubsystem::LogDevelopmentAcceptanceState(const TCHAR* Label) const
{
	const UWorld* World = GetWorld();
	const USOTMObjectiveSubsystem* Objectives = World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<USOTMObjectiveSubsystem>() : nullptr;
	const FSOTMObjectiveData Objective = Objectives
		? Objectives->GetCollectAllForestCoinsObjective() : FSOTMObjectiveData();
	UE_LOG(LogSOTMPhase2, Display,
		TEXT("[Phase2Acceptance] %s coins=%d lifetime=%d unique=%d objective=%d/%d objectiveState=%d lives=%d dead=%d gameOver=%d catch=%d"),
		Label,
		PlayerState ? PlayerState->GetAvailableCoins() : -1,
		PlayerState ? PlayerState->GetLifetimeCoinsCollected() : -1,
		PlayerState ? PlayerState->GetCollectedCoinCount() : -1,
		Objective.CurrentProgress,
		Objective.RequiredProgress,
		static_cast<int32>(Objective.State),
		PlayerState ? PlayerState->GetCurrentLives() : -1,
		PlayerState && PlayerState->IsPlayerDead(),
		PlayerState && PlayerState->IsGameOver(),
		bCatchActive);
}

void USOTMDemoPhase2WorldSubsystem::CaptureDevelopmentEvidence(const FString& Label) const
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (const UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->GetViewportSize(ViewportSize);
		}
	}
	const FString Path = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("Screenshots/WindowsEditor/Phase2"), Label + TEXT(".png"));
	FScreenshotRequest::RequestScreenshot(Path, true, false);
	UE_LOG(LogSOTMPhase2, Display, TEXT("[Phase2Acceptance] Screenshot requested viewport=%.0fx%.0f path=%s"),
		ViewportSize.X, ViewportSize.Y, *Path);
}
#endif

void USOTMDemoPhase2WorldSubsystem::DisableLegacyForestEnemies(TArray<FTransform>& OutSpawnTransforms)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<APawn*> LegacyEnemies;
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn) || Pawn->IsPlayerControlled())
		{
			continue;
		}
		const FString ClassName = Pawn->GetClass()->GetName();
		if (ClassName.Contains(TEXT("BP_AI"), ESearchCase::IgnoreCase) ||
			Pawn->ActorHasTag(TEXT("Isabel")))
		{
			LegacyEnemies.Add(Pawn);
		}
	}
	LegacyEnemies.Sort([](const APawn& Left, const APawn& Right)
	{
		return Left.GetPathName() < Right.GetPathName();
	});

	for (APawn* Pawn : LegacyEnemies)
	{
		OutSpawnTransforms.Add(Pawn->GetActorTransform());
		World->GetTimerManager().ClearAllTimersForObject(Pawn);
		Pawn->SetActorHiddenInGame(true);
		Pawn->SetActorEnableCollision(false);
		Pawn->SetActorTickEnabled(false);
		if (AAIController* Controller = Cast<AAIController>(Pawn->GetController()))
		{
			World->GetTimerManager().ClearAllTimersForObject(Controller);
			Controller->StopMovement();
			Controller->SetActorTickEnabled(false);
			if (UBrainComponent* Brain = Controller->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Phase 2 replaces legacy Forest Isabella copy with Cousin"));
			}
		}
	}
}

void USOTMDemoPhase2WorldSubsystem::SpawnProductionCousins(const TArray<FTransform>& CandidateTransforms)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	const FVector PlayerLocation = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const int32 SpawnCount = CandidateTransforms.IsEmpty()
		? 0 : FMath::Min(SOTMDemoPhase2Private::ProductionCousinCount, CandidateTransforms.Num());
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const int32 SourceIndex = FMath::Min((Index * CandidateTransforms.Num()) / SpawnCount,
			CandidateTransforms.Num() - 1);
		FTransform SpawnTransform = CandidateTransforms[SourceIndex];
		FVector DesiredLocation = SpawnTransform.GetLocation();
		if (FVector::DistSquared2D(DesiredLocation, PlayerLocation) < FMath::Square(650.0f))
		{
			DesiredLocation += FVector(900.0f + Index * 120.0f, 500.0f, 0.0f);
		}
		if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation Projected;
			if (Navigation->ProjectPointToNavigation(DesiredLocation, Projected, FVector(500.0f, 500.0f, 500.0f)))
			{
				DesiredLocation = Projected.Location;
			}
		}
		SpawnTransform.SetLocation(DesiredLocation);
		SpawnTransform.SetScale3D(FVector::OneVector);

		ASOTMCousinCharacter* Cousin = World->SpawnActorDeferred<ASOTMCousinCharacter>(
			ASOTMCousinCharacter::StaticClass(), SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Cousin)
		{
			continue;
		}
		Cousin->SetPresentationVariant(Index % 3);
		UGameplayStatics::FinishSpawningActor(Cousin, SpawnTransform);
		SpawnedCousins.Add(Cousin);
		if (ASOTMCousinAIController* Controller = Cast<ASOTMCousinAIController>(Cousin->GetController()))
		{
			Controller->SetPresentationVariant(Index % 3);
		}
	}
}

void USOTMDemoPhase2WorldSubsystem::NotifyCousinDetected(ASOTMCousinAIController* Controller)
{
	if (!Controller || WarnedControllers.Contains(Controller) || bCatchActive)
	{
		return;
	}
	WarnedControllers.Add(Controller);
	OnCousinWarningChanged.Broadcast(true);
	const int32 VoiceIndex = GetTypeHash(Controller->GetFName()) %
		UE_ARRAY_COUNT(SOTMDemoPhase2Private::CousinDetectVoices);
	const FVector Location = Controller->GetPawn()
		? Controller->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	PlayTemporaryCousinVoice(
		SOTMDemoPhase2Private::CousinDetectVoices[VoiceIndex],
		FText::FromString(SOTMDemoPhase2Private::CousinDetectLines[VoiceIndex]),
		Location, 0.58f);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarningTimer);
		World->GetTimerManager().SetTimer(WarningTimer, this, &ThisClass::HideCousinWarning, 1.8f, false);
	}
}

void USOTMDemoPhase2WorldSubsystem::NotifyCousinEncounterEnded(ASOTMCousinAIController* Controller)
{
	WarnedControllers.Remove(Controller);
}

void USOTMDemoPhase2WorldSubsystem::HideCousinWarning()
{
	OnCousinWarningChanged.Broadcast(false);
}

bool USOTMDemoPhase2WorldSubsystem::TryStartCousinCatch(
	ASOTMCousinCharacter* Cousin,
	AActor* PlayerActor)
{
	if (bCatchActive || !Cousin || !PlayerActor || !PlayerState ||
		PlayerState->IsPlayerDead() || PlayerState->IsGameOver() || !PlayerState->IsBoundPlayerActor(PlayerActor))
	{
		return false;
	}
	USOTMPlayerVitalComponent* Vital = PlayerActor->FindComponentByClass<USOTMPlayerVitalComponent>();
	if (!Vital || Vital->IsDead() || Vital->IsInvulnerable())
	{
		return false;
	}

	bCatchActive = true;
	bCatchImpactApplied = false;
#if !UE_BUILD_SHIPPING
	bDevelopmentAcceptanceCatchObserved = true;
#endif
	CatchingCousin = Cousin;
	CaughtPlayer = PlayerActor;
	HideCousinWarning();
	SuspendAllCousins();
	PlayerState->AcquireInputLock(ESOTMInputLockReason::JumpScare);
	bJumpScareLockHeld = true;

	FVector ToPlayer = PlayerActor->GetActorLocation() - Cousin->GetActorLocation();
	ToPlayer.Z = 0.0f;
	if (!ToPlayer.IsNearlyZero())
	{
		Cousin->SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	}
	CreateCatchCamera(Cousin, PlayerActor);
	if (UAnimInstance* Anim = Cousin->GetMesh() ? Cousin->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, SOTMDemoPhase2Private::CatchMontage))
		{
			Anim->Montage_Play(Montage, 1.0f);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		CatchImpactTimer, this, &ThisClass::PresentCatchImpact, 0.68f, false);
	UE_LOG(LogSOTMPhase2, Display, TEXT("Cousin catch accepted: cousin=%s player=%s; global gate locked."),
		*GetNameSafe(Cousin), *GetNameSafe(PlayerActor));
	return true;
}

void USOTMDemoPhase2WorldSubsystem::PresentCatchImpact()
{
	if (!bCatchActive || bCatchImpactApplied)
	{
		return;
	}
	bCatchImpactApplied = true;
	AActor* PlayerActor = CaughtPlayer.Get();
	USOTMPlayerVitalComponent* Vital = PlayerActor ? PlayerActor->FindComponentByClass<USOTMPlayerVitalComponent>() : nullptr;
	if (!PlayerActor || !Vital || Vital->IsDead() || Vital->IsInvulnerable())
	{
		AbortCatch(TEXT("player unavailable or invulnerable at impact"));
		return;
	}

	if (USoundBase* Scream = LoadObject<USoundBase>(nullptr, SOTMDemoPhase2Private::CatchScream))
	{
		UGameplayStatics::PlaySound2D(this, Scream, 0.9f);
	}
	if (CatchCamera && CatchCamera->GetCameraComponent())
	{
		UCameraComponent* Camera = CatchCamera->GetCameraComponent();
		Camera->SetFieldOfView(50.0f);
		Camera->SetPostProcessBlendWeight(1.0f);
		Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
		Camera->PostProcessSettings.VignetteIntensity = 0.55f;
		Camera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
		Camera->PostProcessSettings.SceneFringeIntensity = 1.0f;
	}

	const float LethalDamage = Vital->GetMaximumHealth() + Vital->GetCurrentHealth() + 1.0f;
	const float Applied = USOTMPlayerBlueprintLibrary::ApplyPlayerDamage(
		this, PlayerActor, LethalDamage,
		CatchingCousin.IsValid() ? CatchingCousin->GetController() : nullptr,
		CatchingCousin.Get());
	UE_LOG(LogSOTMPhase2, Display,
		TEXT("Cousin jump-scare impact requested authoritative lethal damage: requested=%.1f applied=%.1f."),
		LethalDamage, Applied);
	if (Applied <= 0.0f)
	{
		AbortCatch(TEXT("Player System rejected lethal damage"));
	}
}

void USOTMDemoPhase2WorldSubsystem::HandlePlayerDeathStarted(AActor* PlayerActor)
{
	if (!bCatchActive || PlayerActor != CaughtPlayer.Get())
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
		CatchFinishTimer, this, &ThisClass::FinishCatchPresentation, 0.65f, false);
}

void USOTMDemoPhase2WorldSubsystem::FinishCatchPresentation()
{
	RestorePlayerCamera();
	if (PlayerState && bJumpScareLockHeld)
	{
		PlayerState->ReleaseInputLock(ESOTMInputLockReason::JumpScare);
		bJumpScareLockHeld = false;
	}
	GetWorld()->GetTimerManager().SetTimer(
		CameraDestroyTimer, this, &ThisClass::DestroyCatchCamera, 0.30f, false);
}

void USOTMDemoPhase2WorldSubsystem::HandlePlayerRespawned(AActor* PlayerActor)
{
	if (PlayerActor)
	{
		if (USoundBase* Respawn = LoadObject<USoundBase>(nullptr, SOTMDemoPhase2Private::RespawnSound))
		{
			UGameplayStatics::PlaySoundAtLocation(this, Respawn, PlayerActor->GetActorLocation(), 0.58f);
		}
	}
	FinishCatchPresentation();
	bCatchActive = false;
	bCatchImpactApplied = false;
	CatchingCousin.Reset();
	CaughtPlayer.Reset();
	WarnedControllers.Reset();
	GetWorld()->GetTimerManager().SetTimer(
		CousinResetTimer, this, &ThisClass::ResetAllCousins, 1.2f, false);
	UE_LOG(LogSOTMPhase2, Display, TEXT("Cousin encounter cleared after authoritative Player System respawn."));
}

void USOTMDemoPhase2WorldSubsystem::HandleGameOver()
{
	FinishCatchPresentation();
	bCatchActive = false;
	SuspendAllCousins();
}

void USOTMDemoPhase2WorldSubsystem::AbortCatch(const TCHAR* Reason)
{
	GetWorld()->GetTimerManager().ClearTimer(CatchImpactTimer);
	GetWorld()->GetTimerManager().ClearTimer(CatchFinishTimer);
	RestorePlayerCamera();
	DestroyCatchCamera();
	if (PlayerState && bJumpScareLockHeld)
	{
		PlayerState->ReleaseInputLock(ESOTMInputLockReason::JumpScare);
	}
	bJumpScareLockHeld = false;
	bCatchActive = false;
	bCatchImpactApplied = false;
	CatchingCousin.Reset();
	CaughtPlayer.Reset();
	ResetAllCousins();
	UE_LOG(LogSOTMPhase2, Warning, TEXT("Cousin catch aborted safely: %s"), Reason);
}

void USOTMDemoPhase2WorldSubsystem::CreateCatchCamera(
	ASOTMCousinCharacter* Cousin,
	AActor* PlayerActor)
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !PC || !Cousin || !PlayerActor)
	{
		return;
	}
	const FVector Focus = Cousin->GetMesh() && Cousin->GetMesh()->DoesSocketExist(TEXT("head"))
		? Cousin->GetMesh()->GetSocketLocation(TEXT("head"))
		: Cousin->GetActorLocation() + FVector(0.0f, 0.0f, 78.0f);
	FVector TowardPlayer = (PlayerActor->GetActorLocation() - Cousin->GetActorLocation()).GetSafeNormal2D();
	if (TowardPlayer.IsNearlyZero()) TowardPlayer = Cousin->GetActorForwardVector();
	const FVector Desired = Focus + TowardPlayer * 95.0f + FVector::UpVector * 4.0f;
	FActorSpawnParameters Params;
	Params.Owner = Cousin;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CatchCamera = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(), Desired, (Focus - Desired).Rotation(), Params);
	if (CatchCamera && CatchCamera->GetCameraComponent())
	{
		CatchCamera->GetCameraComponent()->SetFieldOfView(64.0f);
		CatchCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
		PC->SetViewTargetWithBlend(
			CatchCamera, 0.20f, EViewTargetBlendFunction::VTBlend_Cubic, 2.0f, true);
	}
}

void USOTMDemoPhase2WorldSubsystem::RestorePlayerCamera()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC && PC->GetPawn() && CatchCamera)
	{
		PC->SetViewTargetWithBlend(
			PC->GetPawn(), 0.20f, EViewTargetBlendFunction::VTBlend_Cubic, 2.0f, false);
	}
}

void USOTMDemoPhase2WorldSubsystem::DestroyCatchCamera()
{
	if (CatchCamera)
	{
		CatchCamera->Destroy();
		CatchCamera = nullptr;
	}
}

void USOTMDemoPhase2WorldSubsystem::SuspendAllCousins()
{
	for (ASOTMCousinCharacter* Cousin : SpawnedCousins)
	{
		if (IsValid(Cousin))
		{
			if (ASOTMCousinAIController* Controller = Cast<ASOTMCousinAIController>(Cousin->GetController()))
			{
				Controller->SuspendForPlayerDeath();
			}
		}
	}
}

void USOTMDemoPhase2WorldSubsystem::ResetAllCousins()
{
	for (ASOTMCousinCharacter* Cousin : SpawnedCousins)
	{
		if (IsValid(Cousin))
		{
			if (ASOTMCousinAIController* Controller = Cast<ASOTMCousinAIController>(Cousin->GetController()))
			{
				Controller->ResetAfterPlayerRespawn();
			}
		}
	}
}
