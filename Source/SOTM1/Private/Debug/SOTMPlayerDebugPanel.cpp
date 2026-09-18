#include "Debug/SOTMPlayerDebugPanel.h"

#if !UE_BUILD_SHIPPING

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/PackageName.h"
#include "SOTMPlayerBlueprintLibrary.h"
#include "SOTMPlayerFoundationWorldSubsystem.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FName NormalizeMapName(const UWorld* World)
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

	FString LockName(const ESOTMInputLockReason Reason)
	{
		switch (Reason)
		{
		case ESOTMInputLockReason::Death: return TEXT("Death");
		case ESOTMInputLockReason::Respawn: return TEXT("Respawn");
		case ESOTMInputLockReason::Cinematic: return TEXT("Cinematic");
		case ESOTMInputLockReason::JumpScare: return TEXT("Jump Scare");
		case ESOTMInputLockReason::PauseMenu: return TEXT("Pause/Menu");
		case ESOTMInputLockReason::GameOver: return TEXT("Game Over");
		case ESOTMInputLockReason::Custom: return TEXT("Custom");
		default: return TEXT("Unknown");
		}
	}

	class SSOTMPlayerDebugPanel final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SSOTMPlayerDebugPanel) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, USOTMPlayerFoundationWorldSubsystem* InOwner)
		{
			OwnerSubsystem = InOwner;
			ChildSlot
			[
				SNew(SBox)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Padding_Lambda([this]() { return FMargin(PanelPosition.X, PanelPosition.Y, 0.0f, 0.0f); })
				[
					SNew(SBox)
					.WidthOverride(370.0f)
					.MaxDesiredHeight(690.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.97f))
						.Padding(8.0f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("PLAYER SYSTEM DEBUG")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
									.ColorAndOpacity(FLinearColor(0.25f, 0.85f, 1.0f))
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(3.0f, 0.0f, 3.0f, 7.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("F10 toggles | Drag the title bar")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FLinearColor(0.65f, 0.7f, 0.75f))
								]
								+ SVerticalBox::Slot().AutoHeight()[MakeRow(TEXT("Damage 25"), &SSOTMPlayerDebugPanel::Damage25, TEXT("Heal 25"), &SSOTMPlayerDebugPanel::Heal25)]
								+ SVerticalBox::Slot().AutoHeight()[MakeRow(TEXT("Set Lives = 1"), &SSOTMPlayerDebugPanel::SetLivesOne, TEXT("Kill Player"), &SSOTMPlayerDebugPanel::KillPlayer)]
								+ SVerticalBox::Slot().AutoHeight()[MakeRow(TEXT("Respawn"), &SSOTMPlayerDebugPanel::Respawn, TEXT("Retry"), &SSOTMPlayerDebugPanel::Retry)]
								+ SVerticalBox::Slot().AutoHeight()[MakeRow(TEXT("Save"), &SSOTMPlayerDebugPanel::SaveState, TEXT("Load"), &SSOTMPlayerDebugPanel::LoadState)]
								+ SVerticalBox::Slot().AutoHeight()[MakeRow(TEXT("Checkpoint Here"), &SSOTMPlayerDebugPanel::CheckpointHere, TEXT("Reset Health"), &SSOTMPlayerDebugPanel::ResetHealth)]
								+ SVerticalBox::Slot().AutoHeight().Padding(3.0f, 8.0f, 3.0f, 3.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("LIVE DEBUG VALUES")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(FLinearColor(0.95f, 0.8f, 0.2f))
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
								[
									SNew(STextBlock)
									.Text(this, &SSOTMPlayerDebugPanel::GetLiveValuesText)
									.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
									.ColorAndOpacity(FLinearColor(0.88f, 0.9f, 0.94f))
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(3.0f, 8.0f, 3.0f, 2.0f)
								[
									SNew(STextBlock)
									.Text_Lambda([this]() { return FText::FromString(StatusMessage); })
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity_Lambda([this]() { return bLastActionSucceeded ? FLinearColor(0.25f, 1.0f, 0.35f) : FLinearColor(1.0f, 0.35f, 0.25f); })
									.AutoWrapText(true)
								]
							]
						]
					]
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
		{
			const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
			const bool bTitle = Local.X >= PanelPosition.X && Local.X <= PanelPosition.X + 370.0f &&
				Local.Y >= PanelPosition.Y && Local.Y <= PanelPosition.Y + 48.0f;
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton && bTitle)
			{
				bDragging = true;
				return FReply::Handled().CaptureMouse(SharedThis(this));
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& Event) override
		{
			if (!bDragging || !HasMouseCapture())
			{
				return FReply::Unhandled();
			}
			PanelPosition += Event.GetCursorDelta();
			PanelPosition.X = FMath::Max(0.0f, PanelPosition.X);
			PanelPosition.Y = FMath::Max(0.0f, PanelPosition.Y);
			Invalidate(EInvalidateWidgetReason::Layout);
			return FReply::Handled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
		{
			if (bDragging && Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				bDragging = false;
				return FReply::Handled().ReleaseMouseCapture();
			}
			return FReply::Unhandled();
		}

	private:
		using FHandler = FReply (SSOTMPlayerDebugPanel::*)();

		TSharedRef<SWidget> MakeRow(const TCHAR* Left, FHandler LeftHandler, const TCHAR* Right, FHandler RightHandler)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
				[SNew(SButton).Text(FText::FromString(Left)).HAlign(HAlign_Center).OnClicked(this, LeftHandler)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f)
				[SNew(SButton).Text(FText::FromString(Right)).HAlign(HAlign_Center).OnClicked(this, RightHandler)];
		}

		UWorld* GetPanelWorld() const { return OwnerSubsystem.IsValid() ? OwnerSubsystem->GetWorld() : nullptr; }
		AActor* GetPlayer() const { const UWorld* World = GetPanelWorld(); const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr; return PC ? PC->GetPawn() : nullptr; }
		USOTMPlayerStateSubsystem* GetState() const { const UWorld* World = GetPanelWorld(); UGameInstance* GI = World ? World->GetGameInstance() : nullptr; return GI ? GI->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr; }
		USOTMPlayerVitalComponent* GetVitals() const { return USOTMPlayerBlueprintLibrary::GetVitalComponent(GetPlayer()); }

		FReply Finish(const TCHAR* Message, const bool bSucceeded)
		{
			bLastActionSucceeded = bSucceeded;
			StatusMessage = bSucceeded ? Message : FString::Printf(TEXT("Action unavailable: %s"), Message);
			if (OwnerSubsystem.IsValid()) { OwnerSubsystem->RefreshDebugPanelInputMode(); }
			return FReply::Handled();
		}

		FReply Damage25() { AActor* Player = GetPlayer(); return Finish(TEXT("Damage 25 applied through gameplay API."), USOTMPlayerBlueprintLibrary::ApplyPlayerDamage(GetPanelWorld(), Player, 25.0f, nullptr, nullptr) > 0.0f); }
		FReply Heal25() { USOTMPlayerVitalComponent* Vitals = GetVitals(); return Finish(TEXT("Heal 25 applied through vital component."), Vitals && Vitals->Heal(25.0f) > 0.0f); }
		FReply SetLivesOne() { USOTMPlayerStateSubsystem* State = GetState(); if (State) { State->SetLivesForDebug(1); } return Finish(TEXT("Lives set to one through player-state API."), State != nullptr); }
		FReply KillPlayer()
		{
			AActor* Player = GetPlayer(); USOTMPlayerVitalComponent* Vitals = GetVitals();
			const float Damage = Vitals ? Vitals->GetMaximumHealth() + Vitals->GetCurrentHealth() : 0.0f;
			return Finish(TEXT("Fatal damage applied through gameplay API."), USOTMPlayerBlueprintLibrary::ApplyPlayerDamage(GetPanelWorld(), Player, Damage, nullptr, nullptr) > 0.0f);
		}
		FReply Respawn() { USOTMPlayerStateSubsystem* State = GetState(); return Finish(TEXT("Normal checkpoint respawn requested."), State && State->ForceRespawnAtCheckpoint()); }
		FReply Retry() { USOTMPlayerStateSubsystem* State = GetState(); return Finish(TEXT("Game Over retry flow requested."), State && State->RetryFromGameOver()); }
		FReply SaveState() { USOTMPlayerStateSubsystem* State = GetState(); return Finish(TEXT("Player state saved."), State && State->SavePlayerState()); }
		FReply LoadState() { USOTMPlayerStateSubsystem* State = GetState(); return Finish(TEXT("Player state loaded."), State && State->LoadPlayerState(false)); }
		FReply CheckpointHere()
		{
			AActor* Player = GetPlayer(); USOTMPlayerStateSubsystem* State = GetState();
			return Finish(TEXT("Current transform registered as checkpoint."), Player && State && State->ActivateCheckpoint(TEXT("DebugPanelCheckpoint"), NormalizeMapName(GetPanelWorld()), Player->GetActorTransform(), true));
		}
		FReply ResetHealth() { USOTMPlayerVitalComponent* Vitals = GetVitals(); if (Vitals) { Vitals->ResetToFullHealth(); } return Finish(TEXT("Health reset through vital component."), Vitals != nullptr); }

		FText GetLiveValuesText() const
		{
			const USOTMPlayerVitalComponent* Vitals = GetVitals();
			const USOTMPlayerStateSubsystem* State = GetState();
			const FSOTMCheckpointState Checkpoint = State ? State->GetCheckpointState() : FSOTMCheckpointState();
			const AActor* Player = GetPlayer();
			const FVector PlayerLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;
			const FVector CheckpointLocation = Checkpoint.RespawnTransform.GetLocation();
			FString Locks = TEXT("None");
			if (State && State->HasAnyInputLock())
			{
				TArray<FString> Labels;
				for (const ESOTMInputLockReason Reason : State->GetActiveInputLockReasons())
				{
					Labels.Add(FString::Printf(TEXT("%s x%d"), *LockName(Reason), State->GetInputLockCount(Reason)));
				}
				Locks = FString::Join(Labels, TEXT(", "));
			}

			return FText::FromString(FString::Printf(
				TEXT("Current Health: %.0f\nMaximum Health: %.0f\nCurrent Lives: %d\nMaximum Lives: %d\nPlayer Dead: %s\nGame Over: %s\nCurrent Checkpoint: %s\nPlayer Location: X %.0f  Y %.0f  Z %.0f\nCheckpoint Location: X %.0f  Y %.0f  Z %.0f\nInput Locks: %s\nCurrent Map: %s"),
				Vitals ? Vitals->GetCurrentHealth() : 0.0f,
				Vitals ? Vitals->GetMaximumHealth() : 0.0f,
				State ? State->GetCurrentLives() : 0,
				State ? State->GetMaximumLives() : 0,
				State && State->IsPlayerDead() ? TEXT("TRUE") : TEXT("false"),
				State && State->IsGameOver() ? TEXT("TRUE") : TEXT("false"),
				Checkpoint.bIsValid ? *Checkpoint.CheckpointId.ToString() : TEXT("None"),
				PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z,
				CheckpointLocation.X, CheckpointLocation.Y, CheckpointLocation.Z,
				*Locks,
				*NormalizeMapName(GetPanelWorld()).ToString()));
		}

		TWeakObjectPtr<USOTMPlayerFoundationWorldSubsystem> OwnerSubsystem;
		FVector2D PanelPosition = FVector2D(14.0f, 14.0f);
		bool bDragging = false;
		bool bLastActionSucceeded = true;
		FString StatusMessage = TEXT("Ready. All actions use the existing Player System.");
	};

	class FSOTMPlayerDebugInputPreprocessor final : public IInputProcessor
	{
	public:
		explicit FSOTMPlayerDebugInputPreprocessor(USOTMPlayerFoundationWorldSubsystem* InOwner) : OwnerSubsystem(InOwner) {}
		virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& Event) override
		{
			if (Event.GetKey() == EKeys::F10 && OwnerSubsystem.IsValid())
			{
				const UWorld* World = OwnerSubsystem->GetWorld();
				UE_LOG(LogTemp, Display, TEXT("SOTM Player Debug Panel: F10 received, World=%s"),
					World ? *World->GetName() : TEXT("None"));
				OwnerSubsystem->ToggleDebugPanel();
				return true;
			}
			return false;
		}
	private:
		TWeakObjectPtr<USOTMPlayerFoundationWorldSubsystem> OwnerSubsystem;
	};
}

TSharedRef<SWidget> SOTMPlayerDebugPanel::CreatePanel(USOTMPlayerFoundationWorldSubsystem* OwnerSubsystem)
{
	return SNew(SSOTMPlayerDebugPanel, OwnerSubsystem);
}

TSharedRef<IInputProcessor> SOTMPlayerDebugPanel::CreateInputPreprocessor(USOTMPlayerFoundationWorldSubsystem* OwnerSubsystem)
{
	return MakeShared<FSOTMPlayerDebugInputPreprocessor>(OwnerSubsystem);
}

#endif
