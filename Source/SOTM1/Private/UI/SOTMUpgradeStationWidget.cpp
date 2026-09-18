#include "UI/SOTMUpgradeStationWidget.h"

#include "Ability/SOTMPhase3Settings.h"
#include "Demo/SOTMDemoPhase3WorldSubsystem.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "SOTMPlayerStateSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace SOTMUpgradeUIPrivate
{
	const FLinearColor Gold(0.93f, 0.64f, 0.23f, 1.0f);
	const FLinearColor Purple(0.73f, 0.25f, 0.96f, 1.0f);
	const FLinearColor Green(0.32f, 0.84f, 0.22f, 1.0f);
	const FLinearColor Red(0.95f, 0.08f, 0.08f, 1.0f);
	const FLinearColor SoftWhite(0.88f, 0.85f, 0.80f, 1.0f);
}

TSharedRef<SWidget> USOTMUpgradeStationWidget::RebuildWidget()
{
	const USOTMPhase3Settings* Settings = GetDefault<USOTMPhase3Settings>();

	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.74f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(24.0f)
		[
			SNew(SBox)
			.WidthOverride(680.0f)
			.MaxDesiredHeight(670.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(34.0f, 24.0f))
				.BorderBackgroundColor(FLinearColor(0.012f, 0.006f, 0.017f, 0.97f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("TIMMY'S UPGRADE STATION")))
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 25))
						.ColorAndOpacity(SOTMUpgradeUIPrivate::Purple)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 16.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("SPEED BOOST")))
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 31))
						.ColorAndOpacity(SOTMUpgradeUIPrivate::SoftWhite)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SAssignNew(OwnershipText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 22))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Temporarily increases movement speed.\nPress [Q] during Forest gameplay.")))
						.Justification(ETextJustify::Center)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 17))
						.ColorAndOpacity(SOTMUpgradeUIPrivate::SoftWhite)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 16.0f)
					[
						SAssignNew(LevelText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18))
						.ColorAndOpacity(SOTMUpgradeUIPrivate::Purple)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(36.0f, 0.0f, 36.0f, 14.0f)
					[
						SNew(SBorder)
						.Padding(14.0f)
						.BorderBackgroundColor(FLinearColor(0.025f, 0.015f, 0.035f, 0.92f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
							[
								SNew(STextBlock).Text(FText::Format(
									FText::FromString(TEXT("SPEED INCREASE                     +{0}%")),
									FText::AsNumber(FMath::RoundToInt((Settings->SpeedBoostMultiplier - 1.0f) * 100.0f))))
								.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 17))
								.ColorAndOpacity(SOTMUpgradeUIPrivate::Green)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
							[
								SNew(STextBlock).Text(FText::Format(
									FText::FromString(TEXT("DURATION                              {0} SEC")),
									FText::AsNumber(Settings->SpeedBoostDuration)))
								.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 17))
								.ColorAndOpacity(SOTMUpgradeUIPrivate::SoftWhite)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
							[
								SNew(STextBlock).Text(FText::Format(
									FText::FromString(TEXT("COOLDOWN                            {0} SEC")),
									FText::AsNumber(Settings->SpeedBoostCooldown)))
								.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 17))
								.ColorAndOpacity(SOTMUpgradeUIPrivate::SoftWhite)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SAssignNew(AvailableCoinsText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 22))
						.ColorAndOpacity(SOTMUpgradeUIPrivate::Gold)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SAssignNew(CostText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 19))
						.ColorAndOpacity(SOTMUpgradeUIPrivate::Gold)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 10.0f)
					[
						SAssignNew(RequirementText, STextBlock)
						.Justification(ETextJustify::Center)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 17))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(80.0f, 0.0f, 80.0f, 8.0f)
					[
						SAssignNew(UnlockButton, SButton)
						.HAlign(HAlign_Center)
						.IsEnabled_Lambda([this]() { return CanPurchase(); })
						.OnClicked_UObject(this, &ThisClass::HandleUnlockClicked)
						[
							SAssignNew(UnlockButtonText, STextBlock)
							.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 19))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(FText::FromString(TEXT("CLOSE")))
						.OnClicked_UObject(this, &ThisClass::HandleCloseClicked)
					]
				]
			]
		];
}

void USOTMUpgradeStationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	PlayerState = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	ObjectiveState = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOTMObjectiveSubsystem>() : nullptr;
	if (PlayerState)
	{
		PlayerState->OnCoinsChanged.AddUniqueDynamic(this, &ThisClass::HandleCoinsChanged);
		PlayerState->OnSpeedBoostOwnershipChanged.AddUniqueDynamic(this, &ThisClass::HandleOwnershipChanged);
	}
	if (ObjectiveState)
	{
		ObjectiveState->OnObjectiveChanged.AddUniqueDynamic(this, &ThisClass::HandleObjectiveChanged);
	}
	RefreshPresentation();
	SetKeyboardFocus();
}

void USOTMUpgradeStationWidget::NativeDestruct()
{
	if (PlayerState)
	{
		PlayerState->OnCoinsChanged.RemoveDynamic(this, &ThisClass::HandleCoinsChanged);
		PlayerState->OnSpeedBoostOwnershipChanged.RemoveDynamic(this, &ThisClass::HandleOwnershipChanged);
	}
	if (ObjectiveState)
	{
		ObjectiveState->OnObjectiveChanged.RemoveDynamic(this, &ThisClass::HandleObjectiveChanged);
	}
	PlayerState = nullptr;
	ObjectiveState = nullptr;
	Super::NativeDestruct();
}

FReply USOTMUpgradeStationWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::E)
	{
		return HandleCloseClicked();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USOTMUpgradeStationWidget::HandleCoinsChanged(int32 AvailableCoins, int32 LifetimeCoinsCollected)
{
	(void)AvailableCoins;
	(void)LifetimeCoinsCollected;
	RefreshPresentation();
}

void USOTMUpgradeStationWidget::HandleOwnershipChanged(bool bUnlocked, int32 Level)
{
	(void)bUnlocked;
	(void)Level;
	RefreshPresentation();
}

void USOTMUpgradeStationWidget::HandleObjectiveChanged(FSOTMObjectiveData Objective)
{
	(void)Objective;
	RefreshPresentation();
}

FReply USOTMUpgradeStationWidget::HandleUnlockClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (USOTMDemoPhase3WorldSubsystem* Phase3 = World->GetSubsystem<USOTMDemoPhase3WorldSubsystem>())
		{
			LastPurchaseResult = Phase3->TryPurchaseSpeedBoost();
			bHasAttemptedPurchase = true;
		}
	}
	RefreshPresentation();
	return FReply::Handled();
}

FReply USOTMUpgradeStationWidget::HandleCloseClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (USOTMDemoPhase3WorldSubsystem* Phase3 = World->GetSubsystem<USOTMDemoPhase3WorldSubsystem>())
		{
			Phase3->CloseUpgradeUI();
		}
	}
	return FReply::Handled();
}

bool USOTMUpgradeStationWidget::CanPurchase() const
{
	if (!PlayerState || PlayerState->IsSpeedBoostUnlocked())
	{
		return false;
	}
	const FSOTMObjectiveData Objective = ObjectiveState
		? ObjectiveState->GetCollectAllForestCoinsObjective() : FSOTMObjectiveData();
	return Objective.State == ESOTMObjectiveState::Completed &&
		PlayerState->GetAvailableCoins() >= GetDefault<USOTMPhase3Settings>()->SpeedBoostUnlockCost;
}

FText USOTMUpgradeStationWidget::GetRequirementText() const
{
	if (!PlayerState)
	{
		return FText::FromString(TEXT("PLAYER STATE UNAVAILABLE"));
	}
	if (PlayerState->IsSpeedBoostUnlocked())
	{
		return FText::FromString(TEXT("OWNED - READY TO USE"));
	}
	const FSOTMObjectiveData Objective = ObjectiveState
		? ObjectiveState->GetCollectAllForestCoinsObjective() : FSOTMObjectiveData();
	if (Objective.State != ESOTMObjectiveState::Completed)
	{
		return FText::FromString(TEXT("COLLECT ALL COINS FIRST"));
	}
	if (PlayerState->GetAvailableCoins() < GetDefault<USOTMPhase3Settings>()->SpeedBoostUnlockCost)
	{
		return FText::FromString(TEXT("NOT ENOUGH COINS"));
	}
	if (bHasAttemptedPurchase && LastPurchaseResult == ESOTMSpeedBoostPurchaseResult::SaveFailed)
	{
		return FText::FromString(TEXT("SAVE FAILED - NOTHING WAS DEDUCTED"));
	}
	return FText::FromString(TEXT("REQUIREMENTS COMPLETE"));
}

void USOTMUpgradeStationWidget::RefreshPresentation()
{
	if (!OwnershipText || !LevelText || !AvailableCoinsText || !CostText || !RequirementText || !UnlockButtonText)
	{
		return;
	}
	const bool bOwned = PlayerState && PlayerState->IsSpeedBoostUnlocked();
	const int32 Level = PlayerState ? PlayerState->GetSpeedBoostLevel() : 0;
	const int32 Available = PlayerState ? PlayerState->GetAvailableCoins() : 0;
	const int32 Cost = GetDefault<USOTMPhase3Settings>()->SpeedBoostUnlockCost;
	const FText Requirement = GetRequirementText();

	OwnershipText->SetText(FText::FromString(bOwned ? TEXT("OWNED") : TEXT("LOCKED")));
	OwnershipText->SetColorAndOpacity(bOwned ? SOTMUpgradeUIPrivate::Green : SOTMUpgradeUIPrivate::Red);
	LevelText->SetText(FText::Format(FText::FromString(TEXT("LEVEL {0} / 1")), FText::AsNumber(Level)));
	AvailableCoinsText->SetText(FText::Format(FText::FromString(TEXT("YOUR COINS   {0}")), FText::AsNumber(Available)));
	CostText->SetText(FText::Format(FText::FromString(TEXT("UNLOCK COST   {0}")), FText::AsNumber(Cost)));
	RequirementText->SetText(Requirement);
	RequirementText->SetColorAndOpacity(CanPurchase() || bOwned
		? SOTMUpgradeUIPrivate::Green : SOTMUpgradeUIPrivate::Red);
	UnlockButtonText->SetText(FText::FromString(bOwned ? TEXT("OWNED") : TEXT("UNLOCK SPEED BOOST")));
}
