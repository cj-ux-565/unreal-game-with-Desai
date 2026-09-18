#include "UI/SOTMDemoCompleteWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerStateSubsystem.h"
#include "Styling/CoreStyle.h"

namespace SOTMDemoCompletePrivate
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, const int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetText(Text);
		Widget->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size));
		Widget->SetColorAndOpacity(FSlateColor(Color));
		Widget->SetJustification(ETextJustify::Center);
		Widget->SetAutoWrapText(true);
		return Widget;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FText& Text)
	{
		UButton* Button = Tree->ConstructWidget<UButton>();
		Button->SetBackgroundColor(FLinearColor(0.18f, 0.035f, 0.27f, 0.96f));
		Button->SetContent(MakeText(Tree, Text, 22, FLinearColor(0.97f, 0.80f, 0.36f, 1.0f)));
		return Button;
	}
}

void USOTMDemoCompleteWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}

	// Build before Slate's RebuildWidget pass; NativeConstruct is too late for a
	// native-only WidgetTree because Slate has already consumed its placeholder root.
	WidgetTree->RootWidget = nullptr;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("DemoCompleteRoot"));
	WidgetTree->RootWidget = Canvas;
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("DemoCompleteBackdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.004f, 0.002f, 0.008f, 0.97f));
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Backdrop))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("DemoCompleteContent"));
	Backdrop->SetContent(Content);
	auto Add = [Content](UWidget* Widget, const FMargin InPadding = FMargin(0.0f))
	{
		if (UVerticalBoxSlot* VerticalSlot = Content->AddChildToVerticalBox(Widget))
		{
			VerticalSlot->SetHorizontalAlignment(HAlign_Center);
			VerticalSlot->SetPadding(InPadding);
		}
	};

	Add(WidgetTree->ConstructWidget<USpacer>(), FMargin(0.0f, 90.0f, 0.0f, 0.0f));
	Add(SOTMDemoCompletePrivate::MakeText(
		WidgetTree, NSLOCTEXT("SOTM", "DemoCompleteGameTitle", "THE SECRETS OF THE MANSION"),
		34, FLinearColor(0.72f, 0.52f, 0.92f, 1.0f)), FMargin(20.0f));
	Add(SOTMDemoCompletePrivate::MakeText(
		WidgetTree, NSLOCTEXT("SOTM", "DemoCompleteTitle", "DEMO COMPLETE"),
		56, FLinearColor(0.98f, 0.80f, 0.33f, 1.0f)), FMargin(20.0f));
	Add(SOTMDemoCompletePrivate::MakeText(
		WidgetTree, NSLOCTEXT("SOTM", "DemoCompleteStory", "THE STORY CONTINUES..."),
		25, FLinearColor(0.88f, 0.86f, 0.91f, 1.0f)), FMargin(0.0f, 4.0f, 0.0f, 34.0f));

	UButton* BuyButton = SOTMDemoCompletePrivate::MakeButton(
		WidgetTree, NSLOCTEXT("SOTM", "BuyFullGame", "BUY THE FULL GAME"));
	BuyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBuyFullGame);
	Add(BuyButton, FMargin(260.0f, 8.0f));
	StoreStatusText = SOTMDemoCompletePrivate::MakeText(
		WidgetTree, NSLOCTEXT("SOTM", "ComingSoon", "COMING SOON"),
		17, FLinearColor(0.62f, 0.56f, 0.69f, 1.0f));
	Add(StoreStatusText, FMargin(0.0f, 8.0f, 0.0f, 22.0f));

	UButton* MenuButton = SOTMDemoCompletePrivate::MakeButton(
		WidgetTree, NSLOCTEXT("SOTM", "DemoReturnMainMenu", "RETURN TO MAIN MENU"));
	MenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleReturnToMainMenu);
	Add(MenuButton, FMargin(260.0f, 8.0f));
}

void USOTMDemoCompleteWidget::HandleBuyFullGame()
{
	if (StoreStatusText)
	{
		StoreStatusText->SetText(NSLOCTEXT(
			"SOTM", "StoreLinkPending", "STORE LINK COMING SOON"));
	}
}

void USOTMDemoCompleteWidget::HandleReturnToMainMenu()
{
	if (USOTMPlayerStateSubsystem* PlayerState =
		USOTMPlayerBlueprintLibrary::GetPlayerStateSubsystem(this))
	{
		PlayerState->ReturnToMainMenu();
	}
}
