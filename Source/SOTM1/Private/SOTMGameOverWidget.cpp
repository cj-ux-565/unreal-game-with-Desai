#include "SOTMGameOverWidget.h"

#include "Engine/GameInstance.h"
#include "SOTMPlayerStateSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> USOTMGameOverWidget::RebuildWidget()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(20.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("GAME OVER")))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString(TEXT("Retry from Checkpoint")))
				.OnClicked_Lambda([this]()
				{
					HandleRetryClicked();
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString(TEXT("Main Menu")))
				.OnClicked_Lambda([this]()
				{
					HandleMainMenuClicked();
					return FReply::Handled();
				})
			]
		];
}

void USOTMGameOverWidget::HandleRetryClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USOTMPlayerStateSubsystem* State = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>())
		{
			State->RetryFromGameOver();
		}
	}
}

void USOTMGameOverWidget::HandleMainMenuClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USOTMPlayerStateSubsystem* State = GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>())
		{
			State->ReturnToMainMenu();
		}
	}
}
