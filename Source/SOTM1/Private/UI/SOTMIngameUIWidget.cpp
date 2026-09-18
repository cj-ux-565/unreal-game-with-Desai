#include "UI/SOTMIngameUIWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Demo/SOTMDemoPhase2WorldSubsystem.h"
#include "Demo/SOTMDemoPhase3WorldSubsystem.h"
#include "Demo/SOTMDemoPhase4WorldSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTMHUD, Log, All);

namespace
{
	const FLinearColor PanelColor(0.012f, 0.018f, 0.032f, 0.88f);
	const FLinearColor PrimaryTextColor(0.93f, 0.90f, 0.85f, 1.0f);
	const FLinearColor MutedTextColor(0.38f, 0.37f, 0.38f, 1.0f);
	const FLinearColor PurpleAccent(0.66f, 0.30f, 0.75f, 1.0f);
	const FLinearColor GoldAccent(0.96f, 0.70f, 0.22f, 1.0f);
	const FLinearColor RedAccent(0.86f, 0.06f, 0.08f, 1.0f);

	UTextBlock* CreateText(
		UWidgetTree* Tree,
		const FName Name,
		const FText& Text,
		const int32 Size,
		const FLinearColor& Color)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Widget->SetText(Text);
		Widget->SetColorAndOpacity(FSlateColor(Color));
		Widget->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Widget->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = Size;
		Widget->SetFont(Font);
		return Widget;
	}

	void AddVertical(UVerticalBox* Parent, UWidget* Child, const FMargin& Padding)
	{
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child))
		{
			Slot->SetPadding(Padding);
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
	}

#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<USOTMIngameUIWidget> ActiveDevelopmentHUD;
	bool bPendingObjectivePreview = false;
	bool bPendingUpgradePreview = false;
	bool bPendingBossPreview = false;
	FIntPoint PendingPreviewCaptureResolution = FIntPoint::ZeroValue;

	FAutoConsoleCommand TestObjectiveCommand(
		TEXT("SOTM.HUD.TestObjective"),
		TEXT("Show development-only Objective Panel preview data."),
		FConsoleCommandDelegate::CreateLambda([]
		{
			bPendingObjectivePreview = true;
			if (USOTMIngameUIWidget* HUD = ActiveDevelopmentHUD.Get())
			{
				HUD->ShowDevelopmentObjectivePreview();
				bPendingObjectivePreview = false;
			}
		}));

	FAutoConsoleCommand TestUpgradeCommand(
		TEXT("SOTM.HUD.TestUpgradeProgress"),
		TEXT("Show development-only upgrade requirement/progress preview data."),
		FConsoleCommandDelegate::CreateLambda([]
		{
			bPendingUpgradePreview = true;
			if (USOTMIngameUIWidget* HUD = ActiveDevelopmentHUD.Get())
			{
				HUD->ShowDevelopmentUpgradePreview();
				bPendingUpgradePreview = false;
			}
		}));

	FAutoConsoleCommand TestBossCommand(
		TEXT("SOTM.HUD.TestBossProgress"),
		TEXT("Show development-only Boss Progress preview data."),
		FConsoleCommandDelegate::CreateLambda([]
		{
			bPendingBossPreview = true;
			if (USOTMIngameUIWidget* HUD = ActiveDevelopmentHUD.Get())
			{
				HUD->ShowDevelopmentBossPreview();
				bPendingBossPreview = false;
			}
		}));

	FAutoConsoleCommand ClearPreviewCommand(
		TEXT("SOTM.HUD.ClearPreview"),
		TEXT("Clear all development-only HUD preview data."),
		FConsoleCommandDelegate::CreateLambda([]
		{
			bPendingObjectivePreview = false;
			bPendingUpgradePreview = false;
			bPendingBossPreview = false;
			if (USOTMIngameUIWidget* HUD = ActiveDevelopmentHUD.Get())
			{
				HUD->ClearDevelopmentPreview();
			}
		}));

	FAutoConsoleCommand CapturePreview1080Command(
		TEXT("SOTM.HUD.CapturePreview1080"),
		TEXT("Show all development HUD preview data and capture it at 1920x1080."),
		FConsoleCommandDelegate::CreateLambda([]
		{
			PendingPreviewCaptureResolution = FIntPoint(1920, 1080);
			if (USOTMIngameUIWidget* HUD = ActiveDevelopmentHUD.Get())
			{
				HUD->CaptureDevelopmentPreview(1920, 1080);
				PendingPreviewCaptureResolution = FIntPoint::ZeroValue;
			}
		}));

	FAutoConsoleCommand CapturePreview720Command(
		TEXT("SOTM.HUD.CapturePreview720"),
		TEXT("Show all development HUD preview data and capture it at 1280x720."),
		FConsoleCommandDelegate::CreateLambda([]
		{
			PendingPreviewCaptureResolution = FIntPoint(1280, 720);
			if (USOTMIngameUIWidget* HUD = ActiveDevelopmentHUD.Get())
			{
				HUD->CaptureDevelopmentPreview(1280, 720);
				PendingPreviewCaptureResolution = FIntPoint::ZeroValue;
			}
		}));
#endif
}

void USOTMIngameUIWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureProductionHUD();
	HideForestHealthPresentation();

	BoundPlayerState = USOTMPlayerBlueprintLibrary::GetPlayerStateSubsystem(this);
	if (BoundPlayerState)
	{
		BoundPlayerState->OnCoinsChanged.RemoveDynamic(this, &ThisClass::HandleCoinsChanged);
		BoundPlayerState->OnCoinsChanged.AddDynamic(this, &ThisClass::HandleCoinsChanged);
		BoundPlayerState->OnLivesChanged.RemoveDynamic(this, &ThisClass::HandleLivesChanged);
		BoundPlayerState->OnLivesChanged.AddDynamic(this, &ThisClass::HandleLivesChanged);
		BoundPlayerState->OnPlayerDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		BoundPlayerState->OnPlayerDeathStarted.AddDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		BoundPlayerState->OnPlayerRespawned.RemoveDynamic(this, &ThisClass::HandlePlayerRespawned);
		BoundPlayerState->OnPlayerRespawned.AddDynamic(this, &ThisClass::HandlePlayerRespawned);
		BoundPlayerState->OnGameOver.RemoveDynamic(this, &ThisClass::HandleGameOver);
		BoundPlayerState->OnGameOver.AddDynamic(this, &ThisClass::HandleGameOver);
		BoundPlayerState->OnInputLocksChanged.RemoveDynamic(this, &ThisClass::HandleInputLocksChanged);
		BoundPlayerState->OnInputLocksChanged.AddDynamic(this, &ThisClass::HandleInputLocksChanged);
		BoundPlayerState->OnPhase4ProgressChanged.RemoveDynamic(this, &ThisClass::HandlePhase4ProgressChanged);
		BoundPlayerState->OnPhase4ProgressChanged.AddDynamic(this, &ThisClass::HandlePhase4ProgressChanged);

		RefreshCoinCounter(
			BoundPlayerState->GetAvailableCoins(),
			BoundPlayerState->GetLifetimeCoinsCollected(),
			false);
		RefreshLives(BoundPlayerState->GetCurrentLives(), BoundPlayerState->GetMaximumLives(), false);
		HandleInputLocksChanged(
			BoundPlayerState->HasAnyInputLock(),
			BoundPlayerState->GetActiveInputLockReasons());
		HandlePhase4ProgressChanged(
			BoundPlayerState->IsPhase4ChestOpened(), BoundPlayerState->HasPhase4GateKey(),
			BoundPlayerState->IsPhase4GateUnlocked(), BoundPlayerState->IsPhase4DemoCompleted());
	}
	else
	{
		RefreshCoinCounter(0, 0, false);
		RefreshLives(0, 0, false);
	}

	BoundObjectiveState = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USOTMObjectiveSubsystem>() : nullptr;
	if (BoundObjectiveState)
	{
		BoundObjectiveState->OnObjectiveChanged.RemoveDynamic(this, &ThisClass::HandleObjectiveChanged);
		BoundObjectiveState->OnObjectiveChanged.AddDynamic(this, &ThisClass::HandleObjectiveChanged);
		RefreshObjectivePresentation(BoundObjectiveState->GetCollectAllForestCoinsObjective());
	}
	else
	{
		RefreshObjectivePresentation(FSOTMObjectiveData());
	}

	BoundPhase2World = GetWorld() ? GetWorld()->GetSubsystem<USOTMDemoPhase2WorldSubsystem>() : nullptr;
	if (BoundPhase2World)
	{
		BoundPhase2World->OnCousinWarningChanged.RemoveDynamic(this, &ThisClass::HandleCousinWarningChanged);
		BoundPhase2World->OnCousinWarningChanged.AddDynamic(this, &ThisClass::HandleCousinWarningChanged);
	}

	BoundPhase3World = GetWorld() ? GetWorld()->GetSubsystem<USOTMDemoPhase3WorldSubsystem>() : nullptr;
	if (BoundPhase3World)
	{
		BoundPhase3World->OnStationPromptChanged.RemoveDynamic(this, &ThisClass::HandleStationPromptChanged);
		BoundPhase3World->OnStationPromptChanged.AddDynamic(this, &ThisClass::HandleStationPromptChanged);
		BoundPhase3World->OnSpeedBoostStateChanged.RemoveDynamic(this, &ThisClass::HandleSpeedBoostStateChanged);
		BoundPhase3World->OnSpeedBoostStateChanged.AddDynamic(this, &ThisClass::HandleSpeedBoostStateChanged);
		HandleSpeedBoostStateChanged(BoundPhase3World->GetSpeedBoostState(), 0.0f, 0.0f);
	}

	BoundPhase4World = GetWorld() ? GetWorld()->GetSubsystem<USOTMDemoPhase4WorldSubsystem>() : nullptr;
	if (BoundPhase4World)
	{
		BoundPhase4World->OnPromptChanged.RemoveDynamic(this, &ThisClass::HandlePhase4PromptChanged);
		BoundPhase4World->OnPromptChanged.AddDynamic(this, &ThisClass::HandlePhase4PromptChanged);
		BoundPhase4World->OnNotification.RemoveDynamic(this, &ThisClass::HandlePhase4Notification);
		BoundPhase4World->OnNotification.AddDynamic(this, &ThisClass::HandlePhase4Notification);
	}

#if !UE_BUILD_SHIPPING
	ActiveDevelopmentHUD = this;
	if (bPendingObjectivePreview)
	{
		ShowDevelopmentObjectivePreview();
		bPendingObjectivePreview = false;
	}
	if (bPendingUpgradePreview)
	{
		ShowDevelopmentUpgradePreview();
		bPendingUpgradePreview = false;
	}
	if (bPendingBossPreview)
	{
		ShowDevelopmentBossPreview();
		bPendingBossPreview = false;
	}
	if (PendingPreviewCaptureResolution != FIntPoint::ZeroValue)
	{
		const FIntPoint Resolution = PendingPreviewCaptureResolution;
		PendingPreviewCaptureResolution = FIntPoint::ZeroValue;
		CaptureDevelopmentPreview(Resolution.X, Resolution.Y);
	}
#endif
}

void USOTMIngameUIWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoinPulseTimerHandle);
		World->GetTimerManager().ClearTimer(LivesPulseTimerHandle);
		World->GetTimerManager().ClearTimer(Phase4NotificationTimerHandle);
	}

	if (BoundPlayerState)
	{
		BoundPlayerState->OnCoinsChanged.RemoveDynamic(this, &ThisClass::HandleCoinsChanged);
		BoundPlayerState->OnLivesChanged.RemoveDynamic(this, &ThisClass::HandleLivesChanged);
		BoundPlayerState->OnPlayerDeathStarted.RemoveDynamic(this, &ThisClass::HandlePlayerDeathStarted);
		BoundPlayerState->OnPlayerRespawned.RemoveDynamic(this, &ThisClass::HandlePlayerRespawned);
		BoundPlayerState->OnGameOver.RemoveDynamic(this, &ThisClass::HandleGameOver);
		BoundPlayerState->OnInputLocksChanged.RemoveDynamic(this, &ThisClass::HandleInputLocksChanged);
		BoundPlayerState->OnPhase4ProgressChanged.RemoveDynamic(this, &ThisClass::HandlePhase4ProgressChanged);
	}
	BoundPlayerState = nullptr;
	if (BoundObjectiveState)
	{
		BoundObjectiveState->OnObjectiveChanged.RemoveDynamic(this, &ThisClass::HandleObjectiveChanged);
	}
	if (BoundPhase2World)
	{
		BoundPhase2World->OnCousinWarningChanged.RemoveDynamic(this, &ThisClass::HandleCousinWarningChanged);
	}
	if (BoundPhase3World)
	{
		BoundPhase3World->OnStationPromptChanged.RemoveDynamic(this, &ThisClass::HandleStationPromptChanged);
		BoundPhase3World->OnSpeedBoostStateChanged.RemoveDynamic(this, &ThisClass::HandleSpeedBoostStateChanged);
	}
	if (BoundPhase4World)
	{
		BoundPhase4World->OnPromptChanged.RemoveDynamic(this, &ThisClass::HandlePhase4PromptChanged);
		BoundPhase4World->OnNotification.RemoveDynamic(this, &ThisClass::HandlePhase4Notification);
	}
	BoundObjectiveState = nullptr;
	BoundPhase2World = nullptr;
	BoundPhase3World = nullptr;
	BoundPhase4World = nullptr;

#if !UE_BUILD_SHIPPING
	if (ActiveDevelopmentHUD.Get() == this)
	{
		ActiveDevelopmentHUD.Reset();
	}
#endif

	Super::NativeDestruct();
}

void USOTMIngameUIWidget::EnsureProductionHUD()
{
	if (ObjectivePanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		UE_LOG(LogSOTMHUD, Error, TEXT("Production WBP_IngameUI root is not a CanvasPanel."));
		return;
	}

	ObjectivePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SOTM_ObjectivePanel"));
	ObjectivePanel->SetBrushColor(FLinearColor(0.008f, 0.009f, 0.012f, 0.91f));
	ObjectivePanel->SetPadding(FMargin(22.0f, 18.0f));
	UVerticalBox* ObjectiveContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_ObjectiveContent"));
	ObjectivePanel->SetContent(ObjectiveContent);

	UTextBlock* PanelTitle = CreateText(
		WidgetTree, TEXT("SOTM_ObjectivePanelTitle"),
		NSLOCTEXT("SOTM", "Phase2ObjectivesTitle", "OBJECTIVES"), 25, FLinearColor(0.92f, 0.75f, 0.53f, 1.0f));
	PanelTitle->SetJustification(ETextJustify::Center);
	AddVertical(ObjectiveContent, PanelTitle, FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	CurrentObjectiveSection = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_CurrentObjectiveSection"));
	CurrentObjectiveText = CreateText(
		WidgetTree, TEXT("SOTM_CurrentObjectiveText"), FText::GetEmpty(), 19, PrimaryTextColor);
	CurrentObjectiveText->SetAutoWrapText(true);
	CurrentObjectiveText->SetWrapTextAt(330.0f);
	ObjectiveProgressText = CreateText(
		WidgetTree, TEXT("SOTM_ObjectiveProgressText"), FText::GetEmpty(), 18, GoldAccent);
	ObjectiveProgressText->SetJustification(ETextJustify::Right);
	AddVertical(CurrentObjectiveSection, CurrentObjectiveText, FMargin(0.0f, 1.0f, 0.0f, 2.0f));
	AddVertical(CurrentObjectiveSection, ObjectiveProgressText, FMargin(0.0f, 0.0f, 0.0f, 13.0f));
	CurrentObjectiveSection->SetVisibility(ESlateVisibility::Collapsed);
	AddVertical(ObjectiveContent, CurrentObjectiveSection, FMargin(0.0f));

	FutureObjectivesText = CreateText(
		WidgetTree, TEXT("SOTM_FutureObjectivesText"),
		NSLOCTEXT("SOTM", "Phase2FutureObjectives", "[LOCKED]  Find the Chest\n\n[LOCKED]  Obtain the Gate Key\n\n[LOCKED]  Reach the Gate"),
		17, MutedTextColor);
	FutureObjectivesText->SetLineHeightPercentage(1.0f);
	AddVertical(ObjectiveContent, FutureObjectivesText, FMargin(0.0f, 0.0f, 0.0f, 15.0f));

	CoinCounterText = CreateText(
		WidgetTree, TEXT("SOTM_CoinCounterText"), FText::GetEmpty(), 21, GoldAccent);
	AddVertical(ObjectiveContent, CoinCounterText, FMargin(0.0f, 7.0f, 0.0f, 4.0f));

	RequiredCoinsSection = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_RequiredCoinsSection"));
	UTextBlock* RequiredHeader = CreateText(
		WidgetTree, TEXT("SOTM_RequiredCoinsHeader"),
		NSLOCTEXT("SOTM", "RequiredCoinsHeader", "COINS NEEDED"), 12, MutedTextColor);
	RequiredCoinsText = CreateText(
		WidgetTree, TEXT("SOTM_RequiredCoinsText"), FText::GetEmpty(), 18, PrimaryTextColor);
	AddVertical(RequiredCoinsSection, RequiredHeader, FMargin(0.0f));
	AddVertical(RequiredCoinsSection, RequiredCoinsText, FMargin(0.0f, 1.0f, 0.0f, 6.0f));
	RequiredCoinsSection->SetVisibility(ESlateVisibility::Collapsed);
	AddVertical(ObjectiveContent, RequiredCoinsSection, FMargin(0.0f));

	UpgradeSection = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_UpgradeSection"));
	UTextBlock* UpgradeHeader = CreateText(
		WidgetTree, TEXT("SOTM_UpgradeHeader"),
		NSLOCTEXT("SOTM", "UpgradeProgressHeader", "UPGRADE PROGRESS"), 12, MutedTextColor);
	USizeBox* UpgradeBarSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SOTM_UpgradeBarSize"));
	UpgradeBarSize->SetHeightOverride(9.0f);
	UpgradeProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("SOTM_UpgradeProgressBar"));
	UpgradeProgressBar->SetFillColorAndOpacity(PurpleAccent);
	UpgradeBarSize->SetContent(UpgradeProgressBar);
	UpgradeDetailText = CreateText(
		WidgetTree, TEXT("SOTM_UpgradeDetailText"), FText::GetEmpty(), 14, PrimaryTextColor);
	AddVertical(UpgradeSection, UpgradeHeader, FMargin(0.0f));
	AddVertical(UpgradeSection, UpgradeBarSize, FMargin(0.0f, 3.0f, 0.0f, 2.0f));
	AddVertical(UpgradeSection, UpgradeDetailText, FMargin(0.0f, 1.0f, 0.0f, 6.0f));
	UpgradeSection->SetVisibility(ESlateVisibility::Collapsed);
	AddVertical(ObjectiveContent, UpgradeSection, FMargin(0.0f));

	MissionTasksHeader = CreateText(
		WidgetTree, TEXT("SOTM_MissionTasksHeader"),
		NSLOCTEXT("SOTM", "MissionTasksHeader", "MISSION TASKS"), 12, MutedTextColor);
	MissionTasksHeader->SetVisibility(ESlateVisibility::Collapsed);
	AddVertical(ObjectiveContent, MissionTasksHeader, FMargin(0.0f, 2.0f, 0.0f, 2.0f));
	MissionTasksContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_MissionTasks"));
	MissionTasksContainer->SetVisibility(ESlateVisibility::Collapsed);
	AddVertical(ObjectiveContent, MissionTasksContainer, FMargin(0.0f));

	USizeBox* ObjectiveSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SOTM_ObjectivePanelSize"));
	ObjectiveSize->SetWidthOverride(410.0f);
	ObjectiveSize->SetContent(ObjectivePanel);
	if (UCanvasPanelSlot* ObjectiveSlot = RootCanvas->AddChildToCanvas(ObjectiveSize))
	{
		ObjectiveSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		ObjectiveSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		ObjectiveSlot->SetPosition(FVector2D(24.0f, 24.0f));
		ObjectiveSlot->SetAutoSize(true);
		ObjectiveSlot->SetZOrder(100);
	}

	TopRightCoinPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_TopRightCoinPanel"));
	TopRightCoinPanel->SetBrushColor(FLinearColor(0.01f, 0.008f, 0.006f, 0.86f));
	TopRightCoinPanel->SetPadding(FMargin(18.0f, 8.0f));
	TopRightCoinText = CreateText(
		WidgetTree, TEXT("SOTM_TopRightCoinText"), FText::GetEmpty(), 26, GoldAccent);
	TopRightCoinPanel->SetContent(TopRightCoinText);
	if (UCanvasPanelSlot* CoinSlot = RootCanvas->AddChildToCanvas(TopRightCoinPanel))
	{
		CoinSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		CoinSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CoinSlot->SetPosition(FVector2D(-24.0f, 24.0f));
		CoinSlot->SetAutoSize(true);
		CoinSlot->SetZOrder(100);
	}

	LivesText = CreateText(
		WidgetTree, TEXT("SOTM_LivesText"), FText::GetEmpty(), 20, PurpleAccent);
	LivesText->SetJustification(ETextJustify::Left);
	if (UCanvasPanelSlot* LivesSlot = RootCanvas->AddChildToCanvas(LivesText))
	{
		LivesSlot->SetAnchors(FAnchors(0.0f, 1.0f));
		LivesSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		LivesSlot->SetPosition(FVector2D(28.0f, -34.0f));
		LivesSlot->SetAutoSize(true);
		LivesSlot->SetZOrder(100);
	}

	SpeedBoostPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_SpeedBoostLockedPanel"));
	SpeedBoostPanel->SetBrushColor(FLinearColor(0.025f, 0.012f, 0.038f, 0.88f));
	SpeedBoostPanel->SetPadding(FMargin(18.0f, 10.0f));
	UVerticalBox* SpeedContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_SpeedBoostContent"));
	SpeedBoostText = CreateText(
		WidgetTree, TEXT("SOTM_SpeedBoostStateText"),
		NSLOCTEXT("SOTM", "SpeedBoostLocked", "SPEED BOOST\nLOCKED"), 17, PurpleAccent);
	SpeedBoostText->SetJustification(ETextJustify::Center);
	AddVertical(SpeedContent, SpeedBoostText, FMargin(0.0f));
	USizeBox* SpeedProgressSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SOTM_SpeedBoostProgressSize"));
	SpeedProgressSize->SetHeightOverride(6.0f);
	SpeedProgressSize->SetWidthOverride(120.0f);
	SpeedBoostProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("SOTM_SpeedBoostProgressBar"));
	SpeedBoostProgressBar->SetFillColorAndOpacity(PurpleAccent);
	SpeedBoostProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	SpeedProgressSize->SetContent(SpeedBoostProgressBar);
	AddVertical(SpeedContent, SpeedProgressSize, FMargin(0.0f, 5.0f, 0.0f, 0.0f));
	SpeedBoostPanel->SetContent(SpeedContent);
	if (UCanvasPanelSlot* SpeedSlot = RootCanvas->AddChildToCanvas(SpeedBoostPanel))
	{
		SpeedSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		SpeedSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		SpeedSlot->SetPosition(FVector2D(-255.0f, -24.0f));
		SpeedSlot->SetAutoSize(true);
		SpeedSlot->SetZOrder(100);
	}

	StationPromptPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_TimmyStationPrompt"));
	StationPromptPanel->SetBrushColor(FLinearColor(0.015f, 0.006f, 0.022f, 0.94f));
	StationPromptPanel->SetPadding(FMargin(24.0f, 12.0f));
	UTextBlock* StationPromptText = CreateText(
		WidgetTree, TEXT("SOTM_TimmyStationPromptText"),
		NSLOCTEXT("SOTM", "TimmyStationPrompt", "[E]  INTERACT\nUPGRADE ABILITIES"),
		19, PurpleAccent);
	StationPromptText->SetJustification(ETextJustify::Center);
	StationPromptPanel->SetContent(StationPromptText);
	StationPromptPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(StationPromptPanel))
	{
		PromptSlot->SetAnchors(FAnchors(0.5f, 0.72f));
		PromptSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PromptSlot->SetPosition(FVector2D::ZeroVector);
		PromptSlot->SetAutoSize(true);
		PromptSlot->SetZOrder(180);
	}

	GateKeyPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_GateKeyLockedPanel"));
	GateKeyPanel->SetBrushColor(FLinearColor(0.018f, 0.012f, 0.022f, 0.88f));
	GateKeyPanel->SetPadding(FMargin(20.0f, 10.0f));
	GateKeyText = CreateText(
		WidgetTree, TEXT("SOTM_GateKeyLockedText"),
		NSLOCTEXT("SOTM", "GateKeyNotAcquired", "GATE KEY\nNOT ACQUIRED"), 17, PurpleAccent);
	GateKeyText->SetJustification(ETextJustify::Center);
	GateKeyPanel->SetContent(GateKeyText);
	if (UCanvasPanelSlot* GateSlot = RootCanvas->AddChildToCanvas(GateKeyPanel))
	{
		GateSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		GateSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		GateSlot->SetPosition(FVector2D(-24.0f, -24.0f));
		GateSlot->SetAutoSize(true);
		GateSlot->SetZOrder(100);
	}

	Phase4PromptPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_Phase4PromptPanel"));
	Phase4PromptPanel->SetBrushColor(FLinearColor(0.012f, 0.006f, 0.020f, 0.94f));
	Phase4PromptPanel->SetPadding(FMargin(24.0f, 13.0f));
	Phase4PromptText = CreateText(
		WidgetTree, TEXT("SOTM_Phase4PromptText"), FText::GetEmpty(), 20, GoldAccent);
	Phase4PromptText->SetJustification(ETextJustify::Center);
	Phase4PromptPanel->SetContent(Phase4PromptText);
	Phase4PromptPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(Phase4PromptPanel))
	{
		PromptSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		PromptSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		PromptSlot->SetPosition(FVector2D(0.0f, -88.0f));
		PromptSlot->SetAutoSize(true);
		PromptSlot->SetZOrder(260);
	}

	Phase4NotificationPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_Phase4NotificationPanel"));
	Phase4NotificationPanel->SetBrushColor(FLinearColor(0.055f, 0.012f, 0.09f, 0.95f));
	Phase4NotificationPanel->SetPadding(FMargin(34.0f, 20.0f));
	UVerticalBox* NotificationContent = WidgetTree->ConstructWidget<UVerticalBox>();
	Phase4NotificationTitle = CreateText(
		WidgetTree, TEXT("SOTM_Phase4NotificationTitle"), FText::GetEmpty(), 25, GoldAccent);
	Phase4NotificationTitle->SetJustification(ETextJustify::Center);
	Phase4NotificationDetail = CreateText(
		WidgetTree, TEXT("SOTM_Phase4NotificationDetail"), FText::GetEmpty(), 17, PrimaryTextColor);
	Phase4NotificationDetail->SetJustification(ETextJustify::Center);
	AddVertical(NotificationContent, Phase4NotificationTitle, FMargin(0.0f, 0.0f, 0.0f, 7.0f));
	AddVertical(NotificationContent, Phase4NotificationDetail, FMargin(0.0f));
	Phase4NotificationPanel->SetContent(NotificationContent);
	Phase4NotificationPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* NoticeSlot = RootCanvas->AddChildToCanvas(Phase4NotificationPanel))
	{
		NoticeSlot->SetAnchors(FAnchors(0.5f, 0.20f));
		NoticeSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		NoticeSlot->SetPosition(FVector2D(0.0f, 0.0f));
		NoticeSlot->SetAutoSize(true);
		NoticeSlot->SetZOrder(270);
	}

	CousinWarningPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_CousinWarningPanel"));
	CousinWarningPanel->SetBrushColor(FLinearColor(0.08f, 0.005f, 0.008f, 0.91f));
	CousinWarningPanel->SetPadding(FMargin(24.0f, 14.0f));
	UTextBlock* WarningText = CreateText(
		WidgetTree, TEXT("SOTM_CousinWarningText"),
		NSLOCTEXT("SOTM", "CousinSpotted", "COUSIN SPOTTED!\nHide or run before it catches you!"),
		21, RedAccent);
	WarningText->SetJustification(ETextJustify::Center);
	CousinWarningPanel->SetContent(WarningText);
	CousinWarningPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* WarningSlot = RootCanvas->AddChildToCanvas(CousinWarningPanel))
	{
		WarningSlot->SetAnchors(FAnchors(1.0f, 0.55f));
		WarningSlot->SetAlignment(FVector2D(1.0f, 0.5f));
		WarningSlot->SetPosition(FVector2D(-24.0f, 0.0f));
		WarningSlot->SetAutoSize(true);
		WarningSlot->SetZOrder(200);
	}

	BossPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SOTM_BossPanel"));
	BossPanel->SetBrushColor(PanelColor);
	BossPanel->SetPadding(FMargin(18.0f, 10.0f));
	UVerticalBox* BossContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_BossContent"));
	BossPanel->SetContent(BossContent);
	BossNameText = CreateText(
		WidgetTree, TEXT("SOTM_BossNameText"), FText::GetEmpty(), 18, PrimaryTextColor);
	BossNameText->SetJustification(ETextJustify::Center);
	USizeBox* BossBarSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SOTM_BossBarSize"));
	BossBarSize->SetWidthOverride(500.0f);
	BossBarSize->SetHeightOverride(12.0f);
	BossProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("SOTM_BossProgressBar"));
	BossProgressBar->SetFillColorAndOpacity(FLinearColor(0.50f, 0.08f, 0.68f, 1.0f));
	BossBarSize->SetContent(BossProgressBar);
	AddVertical(BossContent, BossNameText, FMargin(0.0f, 0.0f, 0.0f, 5.0f));
	AddVertical(BossContent, BossBarSize, FMargin(0.0f));
	BossPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* BossSlot = RootCanvas->AddChildToCanvas(BossPanel))
	{
		BossSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		BossSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		BossSlot->SetPosition(FVector2D(0.0f, 32.0f));
		BossSlot->SetAutoSize(true);
		BossSlot->SetZOrder(110);
	}
}

void USOTMIngameUIWidget::HandleCoinsChanged(const int32 AvailableCoins, const int32 LifetimeCoinsCollected)
{
	RefreshCoinCounter(AvailableCoins, LifetimeCoinsCollected, true);
}

void USOTMIngameUIWidget::HandleLivesChanged(const int32 CurrentLives, const int32 MaximumLives)
{
	RefreshLives(CurrentLives, MaximumLives, true);
}

void USOTMIngameUIWidget::RefreshCoinCounter(
	const int32 AvailableCoins,
	const int32 LifetimeCoinsCollected,
	const bool bPlayFeedback)
{
	if (!CoinCounterText)
	{
		return;
	}

	const int32 SafeAvailable = FMath::Max(0, AvailableCoins);
	const int32 SafeLifetime = FMath::Max(0, LifetimeCoinsCollected);
	const bool bIncreased = DisplayedAvailableCoins != INDEX_NONE && SafeAvailable > DisplayedAvailableCoins;
	DisplayedAvailableCoins = SafeAvailable;
	CoinCounterText->SetText(FText::Format(
		NSLOCTEXT("SOTM", "CoinCounterFormat", "COINS COLLECTED   {0}"),
		FText::AsNumber(SafeLifetime)));
	if (TopRightCoinText)
	{
		TopRightCoinText->SetText(FText::Format(
			NSLOCTEXT("SOTM", "TopRightCoinCounterFormat", "COINS   {0}"),
			FText::AsNumber(SafeAvailable)));
	}

	if (bPlayFeedback && bIncreased)
	{
		FWidgetTransform Pulse;
		Pulse.Scale = FVector2D(1.10f, 1.10f);
		CoinCounterText->SetRenderTransform(Pulse);
		CoinCounterText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.96f, 0.50f, 1.0f)));
		if (TopRightCoinText)
		{
			TopRightCoinText->SetRenderTransform(Pulse);
			TopRightCoinText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.96f, 0.50f, 1.0f)));
		}
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CoinPulseTimerHandle);
			World->GetTimerManager().SetTimer(
				CoinPulseTimerHandle, this, &ThisClass::FinishCoinPulse, 0.16f, false);
		}
	}
}

void USOTMIngameUIWidget::RefreshLives(
	const int32 CurrentLives,
	const int32 MaximumLives,
	const bool bPlayFeedback)
{
	if (!LivesText)
	{
		return;
	}

	const int32 SafeLives = FMath::Max(0, CurrentLives);
	const int32 SafeMaximum = FMath::Max(0, MaximumLives);
	const bool bDecreased = DisplayedLives != INDEX_NONE && SafeLives < DisplayedLives;
	DisplayedLives = SafeLives;
	LivesText->SetText(FText::Format(
		NSLOCTEXT("SOTM", "LivesFormat", "LIVES   {0} / {1}"),
		FText::AsNumber(SafeLives), FText::AsNumber(SafeMaximum)));

	if (bPlayFeedback && bDecreased)
	{
		FWidgetTransform Pulse;
		Pulse.Scale = FVector2D(1.10f, 1.10f);
		LivesText->SetRenderTransform(Pulse);
		LivesText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.22f, 0.22f, 1.0f)));
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(LivesPulseTimerHandle);
			World->GetTimerManager().SetTimer(
				LivesPulseTimerHandle, this, &ThisClass::FinishLivesPulse, 0.24f, false);
		}
	}
}

void USOTMIngameUIWidget::FinishCoinPulse()
{
	if (CoinCounterText)
	{
		CoinCounterText->SetRenderTransform(FWidgetTransform());
		CoinCounterText->SetColorAndOpacity(FSlateColor(GoldAccent));
	}
	if (TopRightCoinText)
	{
		TopRightCoinText->SetRenderTransform(FWidgetTransform());
		TopRightCoinText->SetColorAndOpacity(FSlateColor(GoldAccent));
	}
}

void USOTMIngameUIWidget::FinishLivesPulse()
{
	if (LivesText)
	{
		LivesText->SetRenderTransform(FWidgetTransform());
		LivesText->SetColorAndOpacity(FSlateColor(PurpleAccent));
	}
}

void USOTMIngameUIWidget::HandleObjectiveChanged(const FSOTMObjectiveData Objective)
{
	RefreshObjectivePresentation(Objective);
}

void USOTMIngameUIWidget::RefreshObjectivePresentation(const FSOTMObjectiveData& Objective)
{
	(void)Objective;
	const bool bForestObjectiveActive = BoundObjectiveState && BoundObjectiveState->IsForestObjectiveActive();
	const ESlateVisibility ForestHUDVisibility = bForestObjectiveActive
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;

	if (ObjectivePanel)
	{
		ObjectivePanel->SetVisibility(ForestHUDVisibility);
	}
	if (TopRightCoinPanel)
	{
		TopRightCoinPanel->SetVisibility(ForestHUDVisibility);
	}
	if (SpeedBoostPanel)
	{
		SpeedBoostPanel->SetVisibility(ForestHUDVisibility);
	}
	if (GateKeyPanel)
	{
		GateKeyPanel->SetVisibility(ForestHUDVisibility);
	}
	if (CurrentObjectiveSection)
	{
		CurrentObjectiveSection->SetVisibility(ForestHUDVisibility);
	}

	if (!bForestObjectiveActive || !CurrentObjectiveText || !ObjectiveProgressText)
	{
		return;
	}

	const TArray<FSOTMObjectiveData> Objectives = BoundObjectiveState->GetChapterOneObjectives();
	const FSOTMObjectiveData Active = BoundObjectiveState->GetActiveChapterOneObjective();
	if (!Active.ObjectiveId.IsNone())
	{
		const bool bCompleted = Active.State == ESOTMObjectiveState::Completed;
		CurrentObjectiveText->SetText(FText::Format(
			bCompleted
				? NSLOCTEXT("SOTM", "ObjectiveCompleteFormat", "[COMPLETE]  {0}")
				: NSLOCTEXT("SOTM", "ObjectiveActiveFormat", "[ACTIVE]  {0}"),
			Active.DisplayName));
		CurrentObjectiveText->SetColorAndOpacity(FSlateColor(bCompleted ? GoldAccent : PrimaryTextColor));
		ObjectiveProgressText->SetText(Active.ObjectiveId == USOTMObjectiveSubsystem::CollectAllForestCoinsId
			? FText::Format(NSLOCTEXT("SOTM", "ForestCoinsProgress", "{0} / {1}"),
				FText::AsNumber(FMath::Max(0, Active.CurrentProgress)),
				FText::AsNumber(USOTMObjectiveSubsystem::TotalForestCoins))
			: (bCompleted ? NSLOCTEXT("SOTM", "ObjectiveCompleteShort", "COMPLETE")
				: NSLOCTEXT("SOTM", "ObjectiveInProgress", "IN PROGRESS")));
	}

	if (FutureObjectivesText)
	{
		FString Rows;
		for (const FSOTMObjectiveData& Item : Objectives)
		{
			if (Item.ObjectiveId == Active.ObjectiveId ||
				Item.ObjectiveId == USOTMObjectiveSubsystem::CollectAllForestCoinsId ||
				Item.ObjectiveId == USOTMObjectiveSubsystem::DemoCompleteId)
			{
				continue;
			}
			const TCHAR* Prefix = Item.State == ESOTMObjectiveState::Completed ? TEXT("[DONE]")
				: (Item.State == ESOTMObjectiveState::Active ? TEXT("[ACTIVE]") : TEXT("[LOCKED]"));
			Rows += FString::Printf(TEXT("%s  %s\n\n"), Prefix, *Item.DisplayName.ToString());
		}
		Rows.RemoveFromEnd(TEXT("\n\n"));
		FutureObjectivesText->SetText(FText::FromString(Rows));
	}
}

void USOTMIngameUIWidget::HandlePhase4ProgressChanged(
	const bool bChestOpened,
	const bool bHasGateKey,
	const bool bGateUnlocked,
	const bool bDemoCompleted)
{
	(void)bChestOpened;
	(void)bGateUnlocked;
	(void)bDemoCompleted;
	if (GateKeyText)
	{
		GateKeyText->SetText(bHasGateKey
			? NSLOCTEXT("SOTM", "GateKeyAcquiredHUD", "GATE KEY\nACQUIRED")
			: NSLOCTEXT("SOTM", "GateKeyNotAcquired", "GATE KEY\nNOT ACQUIRED"));
		GateKeyText->SetColorAndOpacity(FSlateColor(bHasGateKey ? GoldAccent : PurpleAccent));
	}
	if (BoundObjectiveState)
	{
		RefreshObjectivePresentation(BoundObjectiveState->GetActiveChapterOneObjective());
	}
}

void USOTMIngameUIWidget::HandlePhase4PromptChanged(const bool bVisible, const FText PromptText)
{
	if (!Phase4PromptPanel || !Phase4PromptText)
	{
		return;
	}
	Phase4PromptText->SetText(PromptText);
	Phase4PromptPanel->SetVisibility(bVisible && !PromptText.IsEmptyOrWhitespace()
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void USOTMIngameUIWidget::HandlePhase4Notification(const FText Title, const FText Detail)
{
	if (!Phase4NotificationPanel || !Phase4NotificationTitle || !Phase4NotificationDetail)
	{
		return;
	}
	Phase4NotificationTitle->SetText(Title);
	Phase4NotificationDetail->SetText(Detail);
	Phase4NotificationPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Phase4NotificationTimerHandle);
		World->GetTimerManager().SetTimer(
			Phase4NotificationTimerHandle, this, &ThisClass::HidePhase4Notification, 4.0f, false);
	}
}

void USOTMIngameUIWidget::HidePhase4Notification()
{
	if (Phase4NotificationPanel)
	{
		Phase4NotificationPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USOTMIngameUIWidget::HideForestHealthPresentation()
{
	const UWorld* World = GetWorld();
	if (!World || FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())) !=
		FName(TEXT("/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1")))
	{
		return;
	}
	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (Widget && Widget->GetName().Contains(TEXT("Health"), ESearchCase::IgnoreCase))
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USOTMIngameUIWidget::HandleCousinWarningChanged(const bool bVisible)
{
	if (!CousinWarningPanel)
	{
		return;
	}

	const bool bForestObjectiveActive = BoundObjectiveState && BoundObjectiveState->IsForestObjectiveActive();
	CousinWarningPanel->SetVisibility(bVisible && bForestObjectiveActive
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
}

void USOTMIngameUIWidget::HandleStationPromptChanged(const bool bVisible)
{
	if (!StationPromptPanel)
	{
		return;
	}
	const bool bForestObjectiveActive = BoundObjectiveState && BoundObjectiveState->IsForestObjectiveActive();
	StationPromptPanel->SetVisibility(bVisible && bForestObjectiveActive
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
}

void USOTMIngameUIWidget::HandleSpeedBoostStateChanged(
	const ESOTMSpeedBoostRuntimeState State,
	const float RemainingSeconds,
	const float NormalizedRemaining)
{
	if (!SpeedBoostText || !SpeedBoostPanel || !SpeedBoostProgressBar)
	{
		return;
	}

	FText StateText;
	FLinearColor StateColor = PurpleAccent;
	bool bShowProgress = false;
	FNumberFormattingOptions CountdownFormat;
	CountdownFormat.SetMaximumFractionalDigits(1);
	CountdownFormat.SetMinimumFractionalDigits(1);
	switch (State)
	{
	case ESOTMSpeedBoostRuntimeState::Ready:
		StateText = NSLOCTEXT("SOTM", "SpeedBoostReady", "SPEED BOOST [Q]\nREADY");
		StateColor = FLinearColor(0.35f, 0.90f, 0.22f, 1.0f);
		break;
	case ESOTMSpeedBoostRuntimeState::Active:
		StateText = FText::Format(
			NSLOCTEXT("SOTM", "SpeedBoostActive", "SPEED BOOST\nACTIVE  {0}s"),
			FText::AsNumber(FMath::Max(0.0f, RemainingSeconds), &CountdownFormat));
		StateColor = FLinearColor(0.20f, 0.62f, 1.0f, 1.0f);
		bShowProgress = true;
		break;
	case ESOTMSpeedBoostRuntimeState::Cooldown:
		StateText = FText::Format(
			NSLOCTEXT("SOTM", "SpeedBoostCooldown", "SPEED BOOST\nCOOLDOWN  {0}s"),
			FText::AsNumber(FMath::Max(0.0f, RemainingSeconds), &CountdownFormat));
		StateColor = PurpleAccent;
		bShowProgress = true;
		break;
	case ESOTMSpeedBoostRuntimeState::Locked:
	default:
		StateText = NSLOCTEXT("SOTM", "SpeedBoostLockedPhase3", "SPEED BOOST\nLOCKED");
		StateColor = RedAccent;
		break;
	}

	SpeedBoostText->SetText(StateText);
	SpeedBoostText->SetColorAndOpacity(FSlateColor(StateColor));
	SpeedBoostPanel->SetBrushColor(State == ESOTMSpeedBoostRuntimeState::Active
		? FLinearColor(0.08f, 0.015f, 0.14f, 0.94f)
		: FLinearColor(0.025f, 0.012f, 0.038f, 0.88f));
	SpeedBoostProgressBar->SetPercent(FMath::Clamp(NormalizedRemaining, 0.0f, 1.0f));
	SpeedBoostProgressBar->SetVisibility(bShowProgress
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
}

void USOTMIngameUIWidget::SetCurrentObjective(const FText& ObjectiveText)
{
	if (!CurrentObjectiveSection || !CurrentObjectiveText)
	{
		return;
	}
	CurrentObjectiveText->SetText(ObjectiveText);
	CurrentObjectiveSection->SetVisibility(
		ObjectiveText.IsEmptyOrWhitespace() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void USOTMIngameUIWidget::ClearCurrentObjective()
{
	SetCurrentObjective(FText::GetEmpty());
}

void USOTMIngameUIWidget::SetRequiredCoins(const int32 CurrentCoins, const int32 RequiredCoins)
{
	if (!RequiredCoinsSection || !RequiredCoinsText)
	{
		return;
	}
	if (RequiredCoins <= 0)
	{
		ClearRequiredCoins();
		return;
	}
	RequiredCoinsText->SetText(FText::Format(
		NSLOCTEXT("SOTM", "RequiredCoinsFormat", "{0} / {1}"),
		FText::AsNumber(FMath::Max(0, CurrentCoins)),
		FText::AsNumber(RequiredCoins)));
	RequiredCoinsSection->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USOTMIngameUIWidget::ClearRequiredCoins()
{
	if (RequiredCoinsSection)
	{
		RequiredCoinsSection->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USOTMIngameUIWidget::SetUpgradeProgress(const float Progress, const FText& DetailText)
{
	if (!UpgradeSection || !UpgradeProgressBar || !UpgradeDetailText)
	{
		return;
	}
	UpgradeProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	UpgradeDetailText->SetText(DetailText);
	UpgradeSection->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USOTMIngameUIWidget::ClearUpgradeProgress()
{
	if (UpgradeSection)
	{
		UpgradeSection->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USOTMIngameUIWidget::SetBossProgress(const FText& BossName, const float NormalizedHealth)
{
	if (!BossPanel || !BossNameText || !BossProgressBar)
	{
		return;
	}
	BossNameText->SetText(FText::Format(
		NSLOCTEXT("SOTM", "BossNameFormat", "BOSS   {0}"), BossName));
	BossProgressBar->SetPercent(FMath::Clamp(NormalizedHealth, 0.0f, 1.0f));
	BossPanel->SetVisibility(
		BossName.IsEmptyOrWhitespace() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void USOTMIngameUIWidget::ClearBossProgress()
{
	if (BossPanel)
	{
		BossPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USOTMIngameUIWidget::SetMissionTask(
	const FName TaskId,
	const FText& TaskText,
	const bool bCompleted)
{
	if (TaskId.IsNone() || TaskText.IsEmptyOrWhitespace() || !MissionTasksContainer || !WidgetTree)
	{
		return;
	}

	UTextBlock* Row = MissionTaskRows.FindRef(TaskId);
	if (!Row)
	{
		Row = CreateText(WidgetTree, NAME_None, FText::GetEmpty(), 15, PrimaryTextColor);
		Row->SetAutoWrapText(true);
		Row->SetWrapTextAt(335.0f);
		AddVertical(MissionTasksContainer, Row, FMargin(0.0f, 1.0f));
		MissionTaskRows.Add(TaskId, Row);
	}

	Row->SetText(FText::Format(
		bCompleted
			? NSLOCTEXT("SOTM", "CompletedTaskFormat", "[DONE]  {0}")
			: NSLOCTEXT("SOTM", "ActiveTaskFormat", "-  {0}"),
		TaskText));
	Row->SetColorAndOpacity(FSlateColor(bCompleted ? MutedTextColor : PrimaryTextColor));
	MissionTasksHeader->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	MissionTasksContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USOTMIngameUIWidget::RemoveMissionTask(const FName TaskId)
{
	if (TObjectPtr<UTextBlock>* Row = MissionTaskRows.Find(TaskId))
	{
		if (MissionTasksContainer && Row->Get())
		{
			MissionTasksContainer->RemoveChild(Row->Get());
		}
		MissionTaskRows.Remove(TaskId);
	}
	if (MissionTaskRows.IsEmpty())
	{
		MissionTasksHeader->SetVisibility(ESlateVisibility::Collapsed);
		MissionTasksContainer->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USOTMIngameUIWidget::ClearMissionTasks()
{
	if (MissionTasksContainer)
	{
		MissionTasksContainer->ClearChildren();
		MissionTasksContainer->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MissionTasksHeader)
	{
		MissionTasksHeader->SetVisibility(ESlateVisibility::Collapsed);
	}
	MissionTaskRows.Reset();
}

void USOTMIngameUIWidget::SetHUDPresentationVisible(const bool bVisible)
{
	bHUDPresentationRequested = bVisible;
	RefreshPresentationVisibility();
}

void USOTMIngameUIWidget::HandlePlayerDeathStarted(AActor* PlayerActor)
{
	(void)PlayerActor;
	bSuppressedByGameplayState = true;
	RefreshPresentationVisibility();
}

void USOTMIngameUIWidget::HandlePlayerRespawned(AActor* PlayerActor)
{
	(void)PlayerActor;
	if (BoundPlayerState)
	{
		RefreshCoinCounter(
			BoundPlayerState->GetAvailableCoins(),
			BoundPlayerState->GetLifetimeCoinsCollected(), false);
		RefreshLives(BoundPlayerState->GetCurrentLives(), BoundPlayerState->GetMaximumLives(), false);
		HandleInputLocksChanged(
			BoundPlayerState->HasAnyInputLock(), BoundPlayerState->GetActiveInputLockReasons());
	}
}

void USOTMIngameUIWidget::HandleGameOver()
{
	bSuppressedByGameplayState = true;
	RefreshPresentationVisibility();
}

void USOTMIngameUIWidget::HandleInputLocksChanged(
	const bool bInputLocked,
	const TArray<ESOTMInputLockReason> ActiveReasons)
{
	(void)bInputLocked;
	bSuppressedByGameplayState = ActiveReasons.Contains(ESOTMInputLockReason::Death) ||
		ActiveReasons.Contains(ESOTMInputLockReason::Respawn) ||
		ActiveReasons.Contains(ESOTMInputLockReason::Cinematic) ||
		ActiveReasons.Contains(ESOTMInputLockReason::JumpScare) ||
		ActiveReasons.Contains(ESOTMInputLockReason::GameOver);
	RefreshPresentationVisibility();
}

void USOTMIngameUIWidget::RefreshPresentationVisibility()
{
	SetVisibility(bHUDPresentationRequested && !bSuppressedByGameplayState
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
}

#if !UE_BUILD_SHIPPING
void USOTMIngameUIWidget::ShowDevelopmentObjectivePreview()
{
	SetCurrentObjective(NSLOCTEXT(
		"SOTM", "HUDPreviewObjective", "Find a path through the forest and recover your stolen power"));
	SetMissionTask(TEXT("PreviewKey"), NSLOCTEXT("SOTM", "HUDPreviewTaskKey", "Find the hidden key"), false);
	SetMissionTask(TEXT("PreviewGate"), NSLOCTEXT("SOTM", "HUDPreviewTaskGate", "Reach the ancient gate"), true);
	UE_LOG(LogSOTMHUD, Display, TEXT("Development-only Objective Panel preview shown; gameplay state unchanged."));
}

void USOTMIngameUIWidget::ShowDevelopmentUpgradePreview()
{
	const int32 Available = BoundPlayerState ? BoundPlayerState->GetAvailableCoins() : 20;
	SetRequiredCoins(Available, 50);
	SetUpgradeProgress(2.0f / 3.0f, NSLOCTEXT("SOTM", "HUDPreviewUpgrade", "2 / 3 fragments recovered"));
	UE_LOG(LogSOTMHUD, Display, TEXT("Development-only Upgrade HUD preview shown; gameplay state unchanged."));
}

void USOTMIngameUIWidget::ShowDevelopmentBossPreview()
{
	SetBossProgress(NSLOCTEXT("SOTM", "HUDPreviewBoss", "ISABEL"), 0.64f);
	UE_LOG(LogSOTMHUD, Display, TEXT("Development-only Boss HUD preview shown; gameplay state unchanged."));
}

void USOTMIngameUIWidget::ClearDevelopmentPreview()
{
	ClearCurrentObjective();
	ClearRequiredCoins();
	ClearUpgradeProgress();
	ClearMissionTasks();
	ClearBossProgress();
	UE_LOG(LogSOTMHUD, Display, TEXT("Development-only HUD preview cleared."));
}

void USOTMIngameUIWidget::CaptureDevelopmentPreview(const int32 Width, const int32 Height)
{
	ShowDevelopmentObjectivePreview();
	ShowDevelopmentUpgradePreview();
	ShowDevelopmentBossPreview();

	if (UWorld* World = GetWorld(); World && GEngine)
	{
		UE_LOG(LogSOTMHUD, Display,
			TEXT("Development HUD capture armed for requested viewport %dx%d."), Width, Height);
		FTimerHandle CaptureTimer;
		World->GetTimerManager().SetTimer(
			CaptureTimer,
			FTimerDelegate::CreateWeakLambda(this, [this]
			{
				if (UWorld* CaptureWorld = GetWorld())
				{
					FVector2D ViewportSize = FVector2D::ZeroVector;
					if (UGameViewportClient* Viewport = CaptureWorld->GetGameViewport())
					{
						Viewport->GetViewportSize(ViewportSize);
					}
					const FString CapturePath = FPaths::Combine(
						FPaths::ProjectSavedDir(), TEXT("Screenshots/WindowsEditor/SOTM_HUD_Preview.png"));
					FScreenshotRequest::RequestScreenshot(CapturePath, true, false);
					UE_LOG(LogSOTMHUD, Display,
						TEXT("Development HUD capture requested after widget construction; viewport=%.0fx%.0f path=%s"),
						ViewportSize.X, ViewportSize.Y, *CapturePath);
				}
			}),
			0.75f,
			false);
	}
}
#endif
