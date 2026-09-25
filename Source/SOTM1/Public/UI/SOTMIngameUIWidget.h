#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ability/SOTMSpeedBoostTypes.h"
#include "Objective/SOTMObjectiveSubsystem.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMIngameUIWidget.generated.h"

class UBorder;
class UImage;
class UOverlay;
class UProgressBar;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
class USOTMDemoPhase2WorldSubsystem;
class USOTMDemoPhase3WorldSubsystem;
class USOTMDemoPhase4WorldSubsystem;
class USOTMObjectiveSubsystem;
class USOTMPlayerStateSubsystem;

/**
 * One objective list row: bone text plus a thin horizontal blood-red
 * strike-through shown only for completed objectives, plus an optional small
 * grungy BloodLines Cross mark as secondary decoration. No gameplay state here.
 */
USTRUCT()
struct FSOTMObjectiveRow
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> Root = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Slash = nullptr;

	/** Small grungy BloodLines completion mark (Cross texture), visible only when completed. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> CompletionMark = nullptr;
};

/**
 * Event-driven presentation adapter for the single production gameplay HUD.
 * Gameplay state remains owned by the Player, Coin, and future feature systems.
 */
UCLASS(Abstract, Blueprintable)
class SOTM1_API USOTMIngameUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Future Objective System presentation contract. Empty text hides the field. */
	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Objective")
	void SetCurrentObjective(const FText& ObjectiveText);

	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Objective")
	void ClearCurrentObjective();

	/** Future Upgrade System presentation contract. Required <= 0 hides the field. */
	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Upgrade")
	void SetRequiredCoins(int32 CurrentCoins, int32 RequiredCoins);

	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Upgrade")
	void ClearRequiredCoins();

	/** Future Upgrade System presentation contract. Progress is normalized 0..1. */
	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Upgrade")
	void SetUpgradeProgress(float Progress, const FText& DetailText);

	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Upgrade")
	void ClearUpgradeProgress();

	/** Future Boss System presentation contract. Boss UI is hidden by default. */
	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Boss")
	void SetBossProgress(const FText& BossName, float NormalizedHealth);

	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Boss")
	void ClearBossProgress();

	/** Add or update a reusable mission-status row without owning mission state. */
	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Objective")
	void SetMissionTask(FName TaskId, const FText& TaskText, bool bCompleted);

	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Objective")
	void RemoveMissionTask(FName TaskId);

	UFUNCTION(BlueprintCallable, Category="SOTM|HUD|Objective")
	void ClearMissionTasks();

	/** Future cinematic/jump-scare systems may request presentation visibility. */
	UFUNCTION(BlueprintCallable, Category="SOTM|HUD")
	void SetHUDPresentationVisible(bool bVisible);

#if !UE_BUILD_SHIPPING
	void ShowDevelopmentObjectivePreview();
	void ShowDevelopmentUpgradePreview();
	void ShowDevelopmentBossPreview();
	void ClearDevelopmentPreview();
	void CaptureDevelopmentPreview(int32 Width, int32 Height);
#endif

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleCoinsChanged(int32 AvailableCoins, int32 LifetimeCoinsCollected);

	UFUNCTION()
	void HandleLivesChanged(int32 CurrentLives, int32 MaximumLives);

	UFUNCTION()
	void HandleObjectiveChanged(FSOTMObjectiveData Objective);

	UFUNCTION()
	void HandleCousinWarningChanged(bool bVisible);

	UFUNCTION()
	void HandleStationPromptChanged(bool bVisible);

	UFUNCTION()
	void HandleSpeedBoostStateChanged(
		ESOTMSpeedBoostRuntimeState State,
		float RemainingSeconds,
		float NormalizedRemaining);

	UFUNCTION()
	void HandlePhase4ProgressChanged(
		bool bChestOpened,
		bool bHasGateKey,
		bool bGateUnlocked,
		bool bDemoCompleted);

	UFUNCTION()
	void HandlePhase4PromptChanged(bool bVisible, FText PromptText);

	UFUNCTION()
	void HandlePhase4Notification(FText Title, FText Detail);

	UFUNCTION()
	void HandlePlayerDeathStarted(AActor* PlayerActor);

	UFUNCTION()
	void HandlePlayerRespawned(AActor* PlayerActor);

	UFUNCTION()
	void HandleGameOver();

	UFUNCTION()
	void HandleInputLocksChanged(bool bInputLocked, TArray<ESOTMInputLockReason> ActiveReasons);

	void EnsureProductionHUD();
	void RefreshCoinCounter(int32 AvailableCoins, int32 LifetimeCoinsCollected, bool bPlayFeedback);
	void RefreshLives(int32 CurrentLives, int32 MaximumLives, bool bPlayFeedback);
	void RefreshObjectivePresentation(const FSOTMObjectiveData& Objective);
	void RefreshPresentationVisibility();
	void FinishCoinPulse();
	void FinishLivesPulse();
	void HidePhase4Notification();
	void HideForestHealthPresentation();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ObjectivePanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> CurrentObjectiveSection;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrentObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CurrentObjectiveSlash;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CurrentObjectiveMark;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> FutureObjectivesContainer;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CoinCounterText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TopRightCoinText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TopRightCoinPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LivesText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> LivesPanel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SpeedBoostPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeedBoostText;

	/** Thin supernatural accent strip above the Speed Boost text; colors per state. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> SpeedAccentStrip;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> SpeedBoostProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> GateKeyPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GateKeyText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Phase4PromptPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Phase4PromptText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Phase4NotificationPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Phase4NotificationTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Phase4NotificationDetail;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CousinWarningPanel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> StationPromptPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RequiredCoinsSection;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RequiredCoinsText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> UpgradeSection;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> UpgradeDetailText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> UpgradeProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MissionTasksHeader;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> MissionTasksContainer;

	UPROPERTY(Transient)
	TMap<FName, FSOTMObjectiveRow> MissionTaskRows;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BossPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> BossProgressBar;

	/** Cached BloodLines textures (null-safe: HUD falls back to solid gothic colors). */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodObjectiveFrameTex = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodNoticeFrameTex = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodOutlineRedTex = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodOutlineGreyTex = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodInputRedTex = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodSlashCrossTex = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodBossBackTex = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BloodBossFillTex = nullptr;

	UTexture2D* LoadBloodLinesTexture(const TCHAR* ObjectPath);
	FSlateBrush MakeBloodFrameBrush(UTexture2D* Tex, const FLinearColor& Tint, float CornerMargin) const;
	void ApplyBloodPanelBrush(UBorder* Panel, UTexture2D* Tex, const FLinearColor& FallbackColor, float CornerMargin);
	void CacheBloodLinesTextures();
	void StyleBossProgressBarWithBloodLines();

	UPROPERTY(Transient)
	TObjectPtr<USOTMPlayerStateSubsystem> BoundPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<USOTMObjectiveSubsystem> BoundObjectiveState;

	UPROPERTY(Transient)
	TObjectPtr<USOTMDemoPhase2WorldSubsystem> BoundPhase2World;

	UPROPERTY(Transient)
	TObjectPtr<USOTMDemoPhase3WorldSubsystem> BoundPhase3World;

	UPROPERTY(Transient)
	TObjectPtr<USOTMDemoPhase4WorldSubsystem> BoundPhase4World;

	FTimerHandle CoinPulseTimerHandle;
	FTimerHandle LivesPulseTimerHandle;
	FTimerHandle Phase4NotificationTimerHandle;
	int32 DisplayedAvailableCoins = INDEX_NONE;
	int32 DisplayedLives = INDEX_NONE;
	bool bHUDPresentationRequested = true;
	bool bSuppressedByGameplayState = false;
};
