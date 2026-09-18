#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SOTMPlayerFoundationWorldSubsystem.generated.h"

class APlayerController;
class UAnimInstance;
class UAnimSequence;
class UGameViewportClient;
class USkeletalMeshComponent;
class USOTMPlayerStateSubsystem;
class UCameraComponent;

/**
 * Runtime-only bootstrap. It discovers the production player pawn and adds the
 * modular vitality component without editing the oversized character Blueprint.
 */
UCLASS()
class SOTM1_API USOTMPlayerFoundationWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

#if !UE_BUILD_SHIPPING
	/** Toggle the isolated Player System demonstration panel. F10 calls this in development builds. */
	void ToggleDebugPanel();

	/** Reapply the correct cursor/input mode after a panel action changes Game Over state. */
	void RefreshDebugPanelInputMode();
#endif

private:
	void HandleActorSpawned(AActor* SpawnedActor);
	void SchedulePlayerBinding();
	void TryBindPlayer();
	UFUNCTION() void HandlePlayerDeathStarted(AActor* PlayerActor);
	UFUNCTION() void HandlePlayerRespawned(AActor* PlayerActor);
	void RestorePlayerAnimation();

	void NormalizeCH1RenderState(UWorld& World, APlayerController* PlayerController, APawn* Pawn);
	void NormalizeProductionCamera(APawn* Pawn);
	static FName GetNormalizedMapPackageName(const UWorld* World);

#if !UE_BUILD_SHIPPING
	bool InitializeDebugPanel(APlayerController* PlayerController);
	void DestroyDebugPanel();
#endif

	FDelegateHandle ActorSpawnedHandle;
	FTimerHandle PlayerBindingTimerHandle;
	TWeakObjectPtr<AActor> LastBoundPlayer;
	TWeakObjectPtr<USkeletalMeshComponent> DeathPresentationMesh;
	TSubclassOf<UAnimInstance> SavedPlayerAnimClass;
	TObjectPtr<UAnimSequence> PlayerDeathAnimation;
	TWeakObjectPtr<USOTMPlayerStateSubsystem> BoundPlayerState;
	bool bDeathAnimationPlaying = false;
	TWeakObjectPtr<UCameraComponent> NormalizedPlayerCamera;

	bool bCH1RenderStateNormalized = false;

#if !UE_BUILD_SHIPPING
	TSharedPtr<class SWidget> DebugPanelWidget;
	TSharedPtr<class IInputProcessor> DebugInputPreprocessor;
	TWeakObjectPtr<APlayerController> DebugPanelPlayerController;
	TWeakObjectPtr<UGameViewportClient> DebugPanelViewportClient;
#endif
};
