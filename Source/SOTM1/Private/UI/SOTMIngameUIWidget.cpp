#include "UI/SOTMIngameUIWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Styling/SlateBrush.h"
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
	const FLinearColor PanelColor(0.010f, 0.012f, 0.020f, 0.80f);
	const FLinearColor PrimaryTextColor(0.90f, 0.87f, 0.82f, 1.0f);
	const FLinearColor MutedTextColor(0.55f, 0.53f, 0.50f, 1.0f);
	const FLinearColor PurpleAccent(0.52f, 0.32f, 0.60f, 1.0f);
	const FLinearColor GoldAccent(0.80f, 0.62f, 0.32f, 1.0f);
	const FLinearColor RedAccent(0.75f, 0.12f, 0.14f, 1.0f);
	// Horror pass: horizontal blood-red strike-through for completed objectives,
	// dim completed text, dim locked-future text, readable ready-state lavender.
	// Strike is intentionally darker/more opaque than the old prototype red.
	const FLinearColor BloodSlashColor(0.55f, 0.07f, 0.09f, 0.90f);
	const FLinearColor CompletedTextColor(0.45f, 0.34f, 0.30f, 1.0f);
	const FLinearColor LockedTextColor(0.32f, 0.30f, 0.29f, 1.0f);
	const FLinearColor ReadyLavender(0.72f, 0.60f, 0.82f, 1.0f);
	const FLinearColor PanelEdgeRed(0.38f, 0.05f, 0.07f, 0.90f);
	// Strong gothic header rule: clearly visible crimson, still restrained.
	const FLinearColor CrimsonRuleColor(0.60f, 0.075f, 0.095f, 0.95f);
	// Supernatural accent strip states for the Speed Boost widget.
	const FLinearColor SpeedReadyGlow(0.58f, 0.36f, 0.68f, 1.0f);
	const FLinearColor SpeedActiveGlow(0.52f, 0.32f, 0.60f, 1.0f);
	const FLinearColor SpeedDimGlow(0.16f, 0.11f, 0.20f, 1.0f);
	// Restrained feedback pulses (no neon/bright prototype colors).
	const FLinearColor CoinPulseGold(0.85f, 0.66f, 0.34f, 1.0f);
	const FLinearColor LivesPulseRed(0.78f, 0.13f, 0.15f, 1.0f);

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

	// Thin blood-red accent strip (2px) used as a horror panel edge.
	void AddEdgeStrip(UWidgetTree* Tree, UVerticalBox* Parent, const FMargin& Padding)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetHeightOverride(2.0f);
		UBorder* Strip = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Strip->SetBrushColor(PanelEdgeRed);
		Strip->SetPadding(FMargin(0.0f));
		Size->SetContent(Strip);
		AddVertical(Parent, Size, Padding);
	}

	// Strong crimson header rule (3px) for hero panels: objective header,
	// notification divider, boss name divider. Clearly visible, not neon.
	void AddCrimsonRule(UWidgetTree* Tree, UVerticalBox* Parent, const FMargin& Padding)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetHeightOverride(3.0f);
		UBorder* Strip = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Strip->SetBrushColor(CrimsonRuleColor);
		Strip->SetPadding(FMargin(0.0f));
		Size->SetContent(Strip);
		AddVertical(Parent, Size, Padding);
	}

	// Objective list row: text plus a perfectly horizontal blood-red
	// strike-through shown only for completed objectives, plus an optional small
	// grungy BloodLines Cross mark as secondary decoration (never the primary).
	// The strike is Angle 0, ~3px, vertically centered: straight at any resolution.
	// CrossTex may be null: the row still works with the solid strike bar.
	FSOTMObjectiveRow CreateSlashRow(
		UWidgetTree* Tree, const FName Name, const FText& Text,
		int32 Size, const FLinearColor& Color, float WrapAt, UTexture2D* CrossTex = nullptr)
	{
		FSOTMObjectiveRow Row;
		Row.Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), Name);
		Row.Text = CreateText(Tree, NAME_None, Text, Size, Color);
		Row.Text->SetAutoWrapText(true);
		Row.Text->SetWrapTextAt(WrapAt);
		if (UOverlaySlot* TextSlot = Row.Root->AddChildToOverlay(Row.Text))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Fill);
			TextSlot->SetVerticalAlignment(VAlign_Fill);
		}
		USizeBox* SlashSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SlashSize->SetHeightOverride(3.0f);
		UBorder* SlashBar = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		SlashBar->SetBrushColor(BloodSlashColor);
		SlashBar->SetPadding(FMargin(0.0f));
		SlashSize->SetContent(SlashBar);
		// Explicitly no rotation: horizontal strike-through, centered on the text.
		SlashSize->SetRenderTransform(FWidgetTransform());
		SlashSize->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SlashSize->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* SlashSlot = Row.Root->AddChildToOverlay(SlashSize))
		{
			SlashSlot->SetHorizontalAlignment(HAlign_Fill);
			SlashSlot->SetVerticalAlignment(VAlign_Center);
		}
		Row.Slash = SlashSize;
		Row.CompletionMark = nullptr;
		if (CrossTex)
		{
			USizeBox* MarkSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			MarkSize->SetWidthOverride(16.0f);
			MarkSize->SetHeightOverride(16.0f);
			UImage* MarkImage = Tree->ConstructWidget<UImage>(UImage::StaticClass());
			FSlateBrush MarkBrush;
			MarkBrush.SetResourceObject(CrossTex);
			MarkBrush.DrawAs = ESlateBrushDrawType::Image;
			MarkBrush.Tiling = ESlateBrushTileType::NoTile;
			MarkBrush.TintColor = FSlateColor(BloodSlashColor);
			MarkImage->SetBrush(MarkBrush);
			MarkImage->SetColorAndOpacity(BloodSlashColor);
			MarkSize->SetContent(MarkImage);
			MarkSize->SetVisibility(ESlateVisibility::Collapsed);
			if (UOverlaySlot* MarkSlot = Row.Root->AddChildToOverlay(MarkSize))
			{
				MarkSlot->SetHorizontalAlignment(HAlign_Right);
				MarkSlot->SetVerticalAlignment(VAlign_Center);
				MarkSlot->SetPadding(FMargin(0.0f, 0.0f, 2.0f, 0.0f));
			}
			Row.CompletionMark = MarkSize;
		}
		return Row;
	}

	void SetSlashRowStyle(FSOTMObjectiveRow& Row, const FText& Text, const FLinearColor& Color, bool bCompleted)
	{
		if (Row.Text)
		{
			Row.Text->SetText(Text);
			Row.Text->SetColorAndOpacity(FSlateColor(Color));
		}
		const ESlateVisibility CompleteVis = bCompleted
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed;
		if (Row.Slash)
		{
			Row.Slash->SetVisibility(CompleteVis);
		}
		if (Row.CompletionMark)
		{
			Row.CompletionMark->SetVisibility(CompleteVis);
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

UTexture2D* USOTMIngameUIWidget::LoadBloodLinesTexture(const TCHAR* ObjectPath)
{
	if (ObjectPath == nullptr || ObjectPath[0] == TEXT('\0'))
	{
		return nullptr;
	}
	// Null-safe soft load: missing/misnamed textures fall back to solid gothic colors.
	UObject* Loaded = StaticLoadObject(UTexture2D::StaticClass(), nullptr, ObjectPath);
	return Cast<UTexture2D>(Loaded);
}

FSlateBrush USOTMIngameUIWidget::MakeBloodFrameBrush(UTexture2D* Tex, const FLinearColor& Tint, float CornerMargin) const
{
	FSlateBrush Brush;
	if (Tex)
	{
		Brush.SetResourceObject(Tex);
		// Box + margin keeps the thin gothic corners from stretching.
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(CornerMargin);
		Brush.Tiling = ESlateBrushTileType::NoTile;
		Brush.TintColor = FSlateColor(Tint);
	}
	return Brush;
}

void USOTMIngameUIWidget::ApplyBloodPanelBrush(UBorder* Panel, UTexture2D* Tex, const FLinearColor& FallbackColor, float CornerMargin)
{
	if (!Panel)
	{
		return;
	}
	if (Tex)
	{
		// White tint preserves the baked BloodLines art (dark grunge + red/grey border).
		Panel->SetBrush(MakeBloodFrameBrush(Tex, FLinearColor::White, CornerMargin));
	}
	else
	{
		Panel->SetBrushColor(FallbackColor);
	}
}

void USOTMIngameUIWidget::CacheBloodLinesTextures()
{
	// BloodLines was a Unity pack: only Texture PNGs were imported. Paths below
	// match the imported .uasset names (note Input_Field with underscore and
	// Rectangle without "=" in the cooked asset names).
	BloodObjectiveFrameTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Frame/Frame_main_menu_red.Frame_main_menu_red"));
	BloodNoticeFrameTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Notice/Frame_notice_v1.Frame_notice_v1"));
	BloodOutlineRedTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Frame/Frame_outline_red.Frame_outline_red"));
	BloodOutlineGreyTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Frame/Frame_outline_v2.Frame_outline_v2"));
	BloodInputRedTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Input_Field/Frame_input_red.Frame_input_red"));
	BloodSlashCrossTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Input_Field/Cross.Cross"));
	BloodBossBackTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Slider/Rectangle/Progress_Bar_Rectangle_empty_v1.Progress_Bar_Rectangle_empty_v1"));
	BloodBossFillTex = LoadBloodLinesTexture(
		TEXT("/Game/UI/BloodLines/Textures/Textures/Slider/Rectangle/Progress_Bar_Rectangle_full_v1.Progress_Bar_Rectangle_full_v1"));
}

void USOTMIngameUIWidget::StyleBossProgressBarWithBloodLines()
{
	if (!BossProgressBar)
	{
		return;
	}
	// Presentation only: health values/percent logic elsewhere is untouched.
	// UE 5.6.1: FProgressBarStyle has no FillColorAndOpacity member; fill tint
	// lives on UProgressBar::FillColorAndOpacity (multiplies FillImage).
	FProgressBarStyle Style = BossProgressBar->GetWidgetStyle();
	if (BloodBossBackTex)
	{
		FSlateBrush BackBrush;
		BackBrush.SetResourceObject(BloodBossBackTex);
		BackBrush.DrawAs = ESlateBrushDrawType::Image;
		BackBrush.Tiling = ESlateBrushTileType::NoTile;
		BackBrush.TintColor = FSlateColor(FLinearColor::White);
		Style.BackgroundImage = BackBrush;
	}
	if (BloodBossFillTex)
	{
		FSlateBrush FillBrush;
		FillBrush.SetResourceObject(BloodBossFillTex);
		FillBrush.DrawAs = ESlateBrushDrawType::Image;
		FillBrush.Tiling = ESlateBrushTileType::NoTile;
		// Texture is already blood-red; a light muted multiply keeps it ominous but restrained.
		FillBrush.TintColor = FSlateColor(FLinearColor(0.72f, 0.62f, 0.62f, 1.0f));
		Style.FillImage = FillBrush;
		BossProgressBar->SetWidgetStyle(Style);
		BossProgressBar->SetFillColorAndOpacity(FLinearColor(0.48f, 0.07f, 0.09f, 1.0f));
		return;
	}
	BossProgressBar->SetWidgetStyle(Style);
	BossProgressBar->SetFillColorAndOpacity(FLinearColor(0.42f, 0.07f, 0.09f, 1.0f));
}

void USOTMIngameUIWidget::EnsureProductionHUD()
{
	if (ObjectivePanel || !WidgetTree)
	{
		return;
	}

	CacheBloodLinesTextures();

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		UE_LOG(LogSOTMHUD, Error, TEXT("Production WBP_IngameUI root is not a CanvasPanel."));
		return;
	}

	ObjectivePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SOTM_ObjectivePanel"));
	ObjectivePanel->SetBrushColor(FLinearColor(0.006f, 0.006f, 0.010f, 0.88f));
	ObjectivePanel->SetPadding(FMargin(18.0f, 14.0f));
	// Gothic integration: baked dark-grunge + thin blood-red border. 9-slice-ish
	// Box margin protects the corner squares from stretching. Falls back to solid.
	ApplyBloodPanelBrush(ObjectivePanel, BloodObjectiveFrameTex,
		FLinearColor(0.006f, 0.006f, 0.010f, 0.88f), 0.08f);
	UVerticalBox* ObjectiveContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_ObjectiveContent"));
	ObjectivePanel->SetContent(ObjectiveContent);

	UTextBlock* PanelTitle = CreateText(
		WidgetTree, TEXT("SOTM_ObjectivePanelTitle"),
		NSLOCTEXT("SOTM", "Phase2ObjectivesTitle", "OBJECTIVE"), 14, MutedTextColor);
	PanelTitle->SetJustification(ETextJustify::Left);
	AddVertical(ObjectiveContent, PanelTitle, FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	AddCrimsonRule(WidgetTree, ObjectiveContent, FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	CurrentObjectiveSection = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_CurrentObjectiveSection"));
	FSOTMObjectiveRow CurrentRow = CreateSlashRow(
		WidgetTree, TEXT("SOTM_CurrentObjectiveRow"), FText::GetEmpty(), 19, PrimaryTextColor, 270.0f, BloodSlashCrossTex);
	CurrentObjectiveText = CurrentRow.Text;
	CurrentObjectiveSlash = CurrentRow.Slash;
	CurrentObjectiveMark = CurrentRow.CompletionMark;
	ObjectiveProgressText = CreateText(
		WidgetTree, TEXT("SOTM_ObjectiveProgressText"), FText::GetEmpty(), 14, GoldAccent);
	ObjectiveProgressText->SetJustification(ETextJustify::Left);
	AddVertical(CurrentObjectiveSection, CurrentRow.Root, FMargin(0.0f, 1.0f, 0.0f, 2.0f));
	AddVertical(CurrentObjectiveSection, ObjectiveProgressText, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	CurrentObjectiveSection->SetVisibility(ESlateVisibility::Collapsed);
	AddVertical(ObjectiveContent, CurrentObjectiveSection, FMargin(0.0f));

	FutureObjectivesContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_FutureObjectives"));
	FutureObjectivesContainer->SetVisibility(ESlateVisibility::Collapsed);
	AddVertical(ObjectiveContent, FutureObjectivesContainer, FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	CoinCounterText = CreateText(
		WidgetTree, TEXT("SOTM_CoinCounterText"), FText::GetEmpty(), 14, GoldAccent);
	AddVertical(ObjectiveContent, CoinCounterText, FMargin(0.0f, 4.0f, 0.0f, 2.0f));

	RequiredCoinsSection = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_RequiredCoinsSection"));
	UTextBlock* RequiredHeader = CreateText(
		WidgetTree, TEXT("SOTM_RequiredCoinsHeader"),
		NSLOCTEXT("SOTM", "RequiredCoinsHeader", "COINS NEEDED"), 12, MutedTextColor);
	RequiredCoinsText = CreateText(
		WidgetTree, TEXT("SOTM_RequiredCoinsText"), FText::GetEmpty(), 14, PrimaryTextColor);
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
	UpgradeBarSize->SetHeightOverride(6.0f);
	UpgradeProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("SOTM_UpgradeProgressBar"));
	UpgradeProgressBar->SetFillColorAndOpacity(PurpleAccent);
	UpgradeBarSize->SetContent(UpgradeProgressBar);
	UpgradeDetailText = CreateText(
		WidgetTree, TEXT("SOTM_UpgradeDetailText"), FText::GetEmpty(), 13, PrimaryTextColor);
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
	ObjectiveSize->SetWidthOverride(320.0f);
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
	TopRightCoinPanel->SetBrushColor(FLinearColor(0.008f, 0.007f, 0.010f, 0.72f));
	TopRightCoinPanel->SetPadding(FMargin(12.0f, 8.0f));
	// Small gothic frame; coins stay dirty-gold typographic (no coin icon in pack).
	ApplyBloodPanelBrush(TopRightCoinPanel, BloodOutlineGreyTex,
		FLinearColor(0.008f, 0.007f, 0.010f, 0.72f), 0.12f);
	TopRightCoinText = CreateText(
		WidgetTree, TEXT("SOTM_TopRightCoinText"), FText::GetEmpty(), 15, GoldAccent);
	TopRightCoinPanel->SetContent(TopRightCoinText);
	if (UCanvasPanelSlot* CoinSlot = RootCanvas->AddChildToCanvas(TopRightCoinPanel))
	{
		CoinSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		CoinSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CoinSlot->SetPosition(FVector2D(-24.0f, 24.0f));
		CoinSlot->SetAutoSize(true);
		CoinSlot->SetZOrder(100);
	}

	// Numeric lives only (no hearts): bone text on a dark gothic backing with
	// a restrained crimson pulse on loss. Format "LIVES 5 / 5" is unchanged.
	LivesPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_LivesPanel"));
	LivesPanel->SetBrushColor(FLinearColor(0.008f, 0.007f, 0.010f, 0.72f));
	LivesPanel->SetPadding(FMargin(12.0f, 7.0f));
	ApplyBloodPanelBrush(LivesPanel, BloodOutlineGreyTex,
		FLinearColor(0.008f, 0.007f, 0.010f, 0.72f), 0.12f);
	LivesText = CreateText(
		WidgetTree, TEXT("SOTM_LivesText"), FText::GetEmpty(), 16, PrimaryTextColor);
	LivesText->SetJustification(ETextJustify::Left);
	LivesPanel->SetContent(LivesText);
	if (UCanvasPanelSlot* LivesSlot = RootCanvas->AddChildToCanvas(LivesPanel))
	{
		LivesSlot->SetAnchors(FAnchors(0.0f, 1.0f));
		LivesSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		LivesSlot->SetPosition(FVector2D(28.0f, -34.0f));
		LivesSlot->SetAutoSize(true);
		LivesSlot->SetZOrder(100);
	}

	SpeedBoostPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_SpeedBoostLockedPanel"));
	SpeedBoostPanel->SetBrushColor(FLinearColor(0.014f, 0.010f, 0.022f, 0.84f));
	SpeedBoostPanel->SetPadding(FMargin(16.0f, 12.0f));
	// Dark BloodLines frame; the purple accent strip below carries the state color.
	ApplyBloodPanelBrush(SpeedBoostPanel, BloodOutlineGreyTex,
		FLinearColor(0.014f, 0.010f, 0.022f, 0.84f), 0.12f);
	UVerticalBox* SpeedContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_SpeedBoostContent"));
	USizeBox* SpeedAccentSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SOTM_SpeedBoostAccentSize"));
	SpeedAccentSize->SetHeightOverride(3.0f);
	SpeedAccentStrip = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_SpeedBoostAccent"));
	SpeedAccentStrip->SetBrushColor(SpeedDimGlow);
	SpeedAccentStrip->SetPadding(FMargin(0.0f));
	SpeedAccentSize->SetContent(SpeedAccentStrip);
	AddVertical(SpeedContent, SpeedAccentSize, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	SpeedBoostText = CreateText(
		WidgetTree, TEXT("SOTM_SpeedBoostStateText"),
		NSLOCTEXT("SOTM", "SpeedBoostLocked", "SPEED BOOST\nLOCKED"), 15, MutedTextColor);
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
		SpeedSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		SpeedSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		SpeedSlot->SetPosition(FVector2D(-24.0f, -110.0f));
		SpeedSlot->SetAutoSize(true);
		SpeedSlot->SetZOrder(100);
	}

	StationPromptPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SOTM_TimmyStationPrompt"));
	StationPromptPanel->SetBrushColor(FLinearColor(0.010f, 0.008f, 0.016f, 0.88f));
	StationPromptPanel->SetPadding(FMargin(24.0f, 12.0f));
	// Prominent blood-red input frame around the bone [E] prompt; small and cinematic.
	ApplyBloodPanelBrush(StationPromptPanel, BloodInputRedTex,
		FLinearColor(0.010f, 0.008f, 0.016f, 0.88f), 0.10f);
	UTextBlock* StationPromptText = CreateText(
		WidgetTree, TEXT("SOTM_TimmyStationPromptText"),
		NSLOCTEXT("SOTM", "TimmyStationPrompt", "[ E ]  INTERACT\nUPGRADE ABILITIES"),
		15, PrimaryTextColor);
	StationPromptText->SetJustification(ETextJustify::Center);
	UVerticalBox* StationPromptContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_TimmyStationPromptContent"));
	AddEdgeStrip(WidgetTree, StationPromptContent, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	AddVertical(StationPromptContent, StationPromptText, FMargin(0.0f));
	StationPromptPanel->SetContent(StationPromptContent);
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
	GateKeyPanel->SetBrushColor(FLinearColor(0.010f, 0.009f, 0.014f, 0.86f));
	GateKeyPanel->SetPadding(FMargin(16.0f, 12.0f));
	// Gothic notice frame so GATE KEY ACQUIRED reads as an important horror beat.
	ApplyBloodPanelBrush(GateKeyPanel, BloodNoticeFrameTex,
		FLinearColor(0.010f, 0.009f, 0.014f, 0.86f), 0.06f);
	GateKeyText = CreateText(
		WidgetTree, TEXT("SOTM_GateKeyLockedText"),
		NSLOCTEXT("SOTM", "GateKeyNotAcquired", "GATE KEY\nNOT ACQUIRED"), 13, MutedTextColor);
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
	Phase4PromptPanel->SetBrushColor(FLinearColor(0.010f, 0.008f, 0.014f, 0.88f));
	Phase4PromptPanel->SetPadding(FMargin(24.0f, 12.0f));
	ApplyBloodPanelBrush(Phase4PromptPanel, BloodInputRedTex,
		FLinearColor(0.010f, 0.008f, 0.014f, 0.88f), 0.10f);
	Phase4PromptText = CreateText(
		WidgetTree, TEXT("SOTM_Phase4PromptText"), FText::GetEmpty(), 15, PrimaryTextColor);
	Phase4PromptText->SetJustification(ETextJustify::Center);
	UVerticalBox* Phase4PromptContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_Phase4PromptContent"));
	AddEdgeStrip(WidgetTree, Phase4PromptContent, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	AddVertical(Phase4PromptContent, Phase4PromptText, FMargin(0.0f));
	Phase4PromptPanel->SetContent(Phase4PromptContent);
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
	Phase4NotificationPanel->SetBrushColor(FLinearColor(0.012f, 0.010f, 0.020f, 0.90f));
	Phase4NotificationPanel->SetPadding(FMargin(28.0f, 16.0f));
	// Prominent gothic modal: dark interior, crimson border, bone text, red divider.
	ApplyBloodPanelBrush(Phase4NotificationPanel, BloodNoticeFrameTex,
		FLinearColor(0.012f, 0.010f, 0.020f, 0.90f), 0.06f);
	UVerticalBox* NotificationContent = WidgetTree->ConstructWidget<UVerticalBox>();
	Phase4NotificationTitle = CreateText(
		WidgetTree, TEXT("SOTM_Phase4NotificationTitle"), FText::GetEmpty(), 20, PrimaryTextColor);
	Phase4NotificationTitle->SetJustification(ETextJustify::Center);
	Phase4NotificationDetail = CreateText(
		WidgetTree, TEXT("SOTM_Phase4NotificationDetail"), FText::GetEmpty(), 14, PrimaryTextColor);
	Phase4NotificationDetail->SetJustification(ETextJustify::Center);
	AddVertical(NotificationContent, Phase4NotificationTitle, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	AddCrimsonRule(WidgetTree, NotificationContent, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
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
	CousinWarningPanel->SetBrushColor(FLinearColor(0.050f, 0.008f, 0.010f, 0.84f));
	CousinWarningPanel->SetPadding(FMargin(20.0f, 12.0f));
	// Danger-only red frame; the only panel allowed a full red outline.
	ApplyBloodPanelBrush(CousinWarningPanel, BloodOutlineRedTex,
		FLinearColor(0.050f, 0.008f, 0.010f, 0.84f), 0.12f);
	UTextBlock* WarningText = CreateText(
		WidgetTree, TEXT("SOTM_CousinWarningText"),
		NSLOCTEXT("SOTM", "CousinSpotted", "COUSIN SPOTTED!\nHide or run before it catches you!"),
		17, RedAccent);
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
	BossPanel->SetPadding(FMargin(20.0f, 10.0f));
	// Ominous but restrained: wide notice bar backing + grungy red/grey bar textures.
	ApplyBloodPanelBrush(BossPanel, BloodNoticeFrameTex, PanelColor, 0.06f);
	UVerticalBox* BossContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("SOTM_BossContent"));
	BossPanel->SetContent(BossContent);
	BossNameText = CreateText(
		WidgetTree, TEXT("SOTM_BossNameText"), FText::GetEmpty(), 16, PrimaryTextColor);
	BossNameText->SetJustification(ETextJustify::Center);
	USizeBox* BossBarSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SOTM_BossBarSize"));
	BossBarSize->SetWidthOverride(480.0f);
	BossBarSize->SetHeightOverride(10.0f);
	BossProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("SOTM_BossProgressBar"));
	BossProgressBar->SetFillColorAndOpacity(FLinearColor(0.42f, 0.07f, 0.09f, 1.0f));
	BossBarSize->SetContent(BossProgressBar);
	StyleBossProgressBarWithBloodLines();
	AddVertical(BossContent, BossNameText, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	AddCrimsonRule(WidgetTree, BossContent, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
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
		NSLOCTEXT("SOTM", "CoinCounterFormat", "{0} collected"),
		FText::AsNumber(SafeLifetime)));
	if (TopRightCoinText)
	{
		TopRightCoinText->SetText(FText::Format(
			NSLOCTEXT("SOTM", "TopRightCoinCounterFormat", "◆   {0}"),
			FText::AsNumber(SafeAvailable)));
	}

	if (bPlayFeedback && bIncreased)
	{
		FWidgetTransform Pulse;
		Pulse.Scale = FVector2D(1.10f, 1.10f);
		CoinCounterText->SetRenderTransform(Pulse);
		CoinCounterText->SetColorAndOpacity(FSlateColor(CoinPulseGold));
		if (TopRightCoinText)
		{
			TopRightCoinText->SetRenderTransform(Pulse);
			TopRightCoinText->SetColorAndOpacity(FSlateColor(CoinPulseGold));
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
		LivesText->SetColorAndOpacity(FSlateColor(LivesPulseRed));
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
		LivesText->SetColorAndOpacity(FSlateColor(PrimaryTextColor));
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
		// Horror presentation: no [ACTIVE]/[COMPLETE] labels. State is carried
		// by style: bone text when active, dark muted text plus a thin
		// blood-red slash when completed.
		const bool bCompleted = Active.State == ESOTMObjectiveState::Completed;
		CurrentObjectiveText->SetText(Active.DisplayName);
		CurrentObjectiveText->SetColorAndOpacity(FSlateColor(bCompleted ? CompletedTextColor : PrimaryTextColor));
		if (CurrentObjectiveSlash)
		{
			CurrentObjectiveSlash->SetVisibility(bCompleted
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		}
		if (CurrentObjectiveMark)
		{
			CurrentObjectiveMark->SetVisibility(bCompleted
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		}
		const bool bIsCoinObjective = Active.ObjectiveId == USOTMObjectiveSubsystem::CollectAllForestCoinsId;
		if (bIsCoinObjective)
		{
			ObjectiveProgressText->SetText(FText::Format(
				NSLOCTEXT("SOTM", "ForestCoinsProgress", "{0} / {1}"),
				FText::AsNumber(FMath::Max(0, Active.CurrentProgress)),
				FText::AsNumber(USOTMObjectiveSubsystem::TotalForestCoins)));
			ObjectiveProgressText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			// No IN PROGRESS/COMPLETE label: the slash carries completion.
			ObjectiveProgressText->SetText(FText::GetEmpty());
			ObjectiveProgressText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (FutureObjectivesContainer)
	{
		FutureObjectivesContainer->ClearChildren();
		bool bHasRows = false;
		for (const FSOTMObjectiveData& Item : Objectives)
		{
			if (Item.ObjectiveId == Active.ObjectiveId ||
				Item.ObjectiveId == USOTMObjectiveSubsystem::CollectAllForestCoinsId ||
				Item.ObjectiveId == USOTMObjectiveSubsystem::DemoCompleteId)
			{
				continue;
			}
			// No [LOCKED]/[DONE] labels: locked rows are dim, completed rows
			// are muted with a blood-red slash.
			const bool bRowCompleted = Item.State == ESOTMObjectiveState::Completed;
			FSOTMObjectiveRow Row = CreateSlashRow(
				WidgetTree, NAME_None, Item.DisplayName, 12,
				bRowCompleted ? CompletedTextColor : LockedTextColor, 270.0f, BloodSlashCrossTex);
			SetSlashRowStyle(Row, Item.DisplayName,
				bRowCompleted ? CompletedTextColor : LockedTextColor, bRowCompleted);
			AddVertical(FutureObjectivesContainer, Row.Root, FMargin(0.0f, 0.0f, 0.0f, 5.0f));
			bHasRows = true;
		}
		FutureObjectivesContainer->SetVisibility(bHasRows
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
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
	FLinearColor StateColor = MutedTextColor;
	bool bShowProgress = false;
	FNumberFormattingOptions CountdownFormat;
	CountdownFormat.SetMaximumFractionalDigits(1);
	CountdownFormat.SetMinimumFractionalDigits(1);
	switch (State)
	{
	case ESOTMSpeedBoostRuntimeState::Ready:
		StateText = NSLOCTEXT("SOTM", "SpeedBoostReady", "SPEED BOOST [Q]\nREADY");
		StateColor = ReadyLavender;
		break;
	case ESOTMSpeedBoostRuntimeState::Active:
		StateText = FText::Format(
			NSLOCTEXT("SOTM", "SpeedBoostActive", "SPEED BOOST\nACTIVE  {0}s"),
			FText::AsNumber(FMath::Max(0.0f, RemainingSeconds), &CountdownFormat));
		StateColor = PrimaryTextColor;
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
		StateColor = MutedTextColor;
		break;
	}

	SpeedBoostText->SetText(StateText);
	SpeedBoostText->SetColorAndOpacity(FSlateColor(StateColor));
	SpeedBoostPanel->SetBrushColor(State == ESOTMSpeedBoostRuntimeState::Active
		? FLinearColor(0.030f, 0.018f, 0.048f, 0.86f)
		: FLinearColor(0.014f, 0.010f, 0.022f, 0.80f));
	// Subtle purple glow ONLY when READY; dim in every other state.
	if (SpeedAccentStrip)
	{
		FLinearColor Accent = SpeedDimGlow;
		if (State == ESOTMSpeedBoostRuntimeState::Ready)
		{
			Accent = SpeedReadyGlow;
		}
		else if (State == ESOTMSpeedBoostRuntimeState::Active)
		{
			Accent = SpeedActiveGlow;
		}
		SpeedAccentStrip->SetBrushColor(Accent);
	}
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
	CurrentObjectiveText->SetColorAndOpacity(FSlateColor(PrimaryTextColor));
	if (CurrentObjectiveSlash)
	{
		CurrentObjectiveSlash->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CurrentObjectiveMark)
	{
		CurrentObjectiveMark->SetVisibility(ESlateVisibility::Collapsed);
	}
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

	FSOTMObjectiveRow* Existing = MissionTaskRows.Find(TaskId);
	if (!Existing || !Existing->Root)
	{
		FSOTMObjectiveRow Row = CreateSlashRow(
			WidgetTree, NAME_None, FText::GetEmpty(), 13, PrimaryTextColor, 270.0f, BloodSlashCrossTex);
		AddVertical(MissionTasksContainer, Row.Root, FMargin(0.0f, 1.0f));
		MissionTaskRows.Add(TaskId, Row);
		Existing = MissionTaskRows.Find(TaskId);
	}
	if (!Existing)
	{
		return;
	}

	// No [DONE]/- labels: completed rows dim with a blood-red slash.
	SetSlashRowStyle(*Existing, TaskText,
		bCompleted ? CompletedTextColor : PrimaryTextColor, bCompleted);
	MissionTasksHeader->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	MissionTasksContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USOTMIngameUIWidget::RemoveMissionTask(const FName TaskId)
{
	if (FSOTMObjectiveRow* Row = MissionTaskRows.Find(TaskId))
	{
		if (MissionTasksContainer && Row->Root)
		{
			MissionTasksContainer->RemoveChild(Row->Root);
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
