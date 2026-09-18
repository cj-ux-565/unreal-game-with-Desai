#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SOTMDemoPhase1WorldSubsystem.generated.h"

class AActor;
class ALevelSequenceActor;
class APlayerController;
class UAudioComponent;
class UGameViewportClient;
class ULevelSequencePlayer;
class USOTMPlayerStateSubsystem;
class UTextBlock;

/**
 * Demo Phase 1 bridge for the production menu and New Game Mansion introduction.
 * It deliberately owns presentation/orchestration only; save, player, coin and AI
 * gameplay logic remain in their existing authoritative systems.
 */
UCLASS()
class SOTM1_API USOTMDemoPhase1WorldSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	static FName GetMapPackageName(const UWorld* World);
	void NormalizeSaveSlotPresentation();
	void TryBeginMansionIntro();
	void BeginMansionIntro(APlayerController* PlayerController, USOTMPlayerStateSubsystem* PlayerState);
	void PresentTimmyOpening();
	void PresentTimmyWarning();
	void PresentTimmyDanger();
	void PresentIsabelArrival();
	void PresentKnockout();
	void PresentDragging();
	void FinishMansionIntro();
	void TravelToForest();
	void PlayIntroDialogueStep();
	void AdvanceIntroDialogue();
	void PlayTemporaryDialogue(const TCHAR* SoundPath, const FText& Speaker, const FText& Line);

	UFUNCTION()
	void HandleTemporaryDialogueFinished();

	void SetSubtitle(const FText& Speaker, const FText& Line);
	void FrameActorWithCinematicCamera(AActor* Subject);
	void CreateSubtitleOverlay();
	void RemoveSubtitleOverlay();
	void StopIsabelGameplayLogic(AActor* IsabelActor) const;
	void CleanupIntro(bool bRestoreCameraFade);

	FTimerHandle SaveSlotNormalizationTimer;
	FTimerHandle IntroStartTimer;
	FTimerHandle DialogueAdvanceTimer;
	FTimerHandle TravelTimer;

	TWeakObjectPtr<APlayerController> IntroPlayerController;
	TWeakObjectPtr<AActor> TimmyActor;
	TWeakObjectPtr<AActor> IsabelActor;
	TWeakObjectPtr<AActor> CinematicCameraActor;
	TWeakObjectPtr<UGameViewportClient> IntroGameViewport;
	TWeakObjectPtr<USOTMPlayerStateSubsystem> IntroPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> IntroSequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> IntroSequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveDialogueAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MansionAmbienceAudio;

	TSharedPtr<class SWidget> SubtitleViewportRoot;
	TSharedPtr<class STextBlock> SubtitleSpeakerText;
	TSharedPtr<class STextBlock> SubtitleLineText;
	int32 IntroDialogueStep = 0;
	bool bIntroRequested = false;
	bool bCinematicLockHeld = false;
};
