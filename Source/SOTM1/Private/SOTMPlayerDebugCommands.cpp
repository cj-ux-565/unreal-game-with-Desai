#include "SOTMPlayerBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"

#if !UE_BUILD_SHIPPING
namespace SOTMPlayerDebugCommands
{
	USOTMPlayerStateSubsystem* GetState(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<USOTMPlayerStateSubsystem>() : nullptr;
	}

	AActor* GetPlayer(UWorld* World)
	{
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	void Damage(const TArray<FString>& Args, UWorld* World)
	{
		const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 25.0f;
		if (AActor* Player = GetPlayer(World))
		{
			UGameplayStatics::ApplyDamage(Player, Amount, nullptr, nullptr, nullptr);
		}
	}

	void Heal(const TArray<FString>& Args, UWorld* World)
	{
		const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 25.0f;
		if (AActor* Player = GetPlayer(World))
		{
			if (USOTMPlayerVitalComponent* Vitals = Player->FindComponentByClass<USOTMPlayerVitalComponent>())
			{
				Vitals->Heal(Amount);
			}
		}
	}

	void SetLives(const TArray<FString>& Args, UWorld* World)
	{
		if (USOTMPlayerStateSubsystem* State = GetState(World))
		{
			State->SetLivesForDebug(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 5);
		}
	}

	void CheckpointHere(const TArray<FString>& Args, UWorld* World)
	{
		USOTMPlayerStateSubsystem* State = GetState(World);
		AActor* Player = GetPlayer(World);
		if (State && Player)
		{
			const FName Id = Args.Num() > 0 ? FName(*Args[0]) : TEXT("DebugCheckpoint");
			State->ActivateCheckpoint(Id, State->GetCheckpointState().MapPackageName, Player->GetActorTransform(), true);
		}
	}

	void Respawn(const TArray<FString>& Args, UWorld* World)
	{
		if (USOTMPlayerStateSubsystem* State = GetState(World))
		{
			State->ForceRespawnAtCheckpoint();
		}
	}

	void Save(const TArray<FString>& Args, UWorld* World)
	{
		if (USOTMPlayerStateSubsystem* State = GetState(World))
		{
			State->SavePlayerState();
		}
	}

	void Load(const TArray<FString>& Args, UWorld* World)
	{
		if (USOTMPlayerStateSubsystem* State = GetState(World))
		{
			State->LoadPlayerState(true);
		}
	}

	void Retry(const TArray<FString>& Args, UWorld* World)
	{
		if (USOTMPlayerStateSubsystem* State = GetState(World))
		{
			State->RetryFromGameOver();
		}
	}

	FAutoConsoleCommandWithWorldAndArgs DamageCommand(
		TEXT("SOTM.Player.Damage"),
		TEXT("Apply standard Unreal damage to the production player. Usage: SOTM.Player.Damage [Amount]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Damage));

	FAutoConsoleCommandWithWorldAndArgs HealCommand(
		TEXT("SOTM.Player.Heal"),
		TEXT("Heal the production player. Usage: SOTM.Player.Heal [Amount]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Heal));

	FAutoConsoleCommandWithWorldAndArgs SetLivesCommand(
		TEXT("SOTM.Player.SetLives"),
		TEXT("Set current lives for development testing. Usage: SOTM.Player.SetLives [Lives]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetLives));

	FAutoConsoleCommandWithWorldAndArgs CheckpointCommand(
		TEXT("SOTM.Player.CheckpointHere"),
		TEXT("Make the player's current transform the active checkpoint."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CheckpointHere));

	FAutoConsoleCommandWithWorldAndArgs RespawnCommand(
		TEXT("SOTM.Player.Respawn"),
		TEXT("Respawn the player at the active checkpoint."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Respawn));

	FAutoConsoleCommandWithWorldAndArgs SaveCommand(
		TEXT("SOTM.Player.Save"),
		TEXT("Write player state into the active Menu System Pro save slot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Save));

	FAutoConsoleCommandWithWorldAndArgs LoadCommand(
		TEXT("SOTM.Player.Load"),
		TEXT("Load player state from the active Menu System Pro save slot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Load));

	FAutoConsoleCommandWithWorldAndArgs RetryCommand(
		TEXT("SOTM.Player.Retry"),
		TEXT("Execute the configured Game Over retry policy."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Retry));
}
#endif
