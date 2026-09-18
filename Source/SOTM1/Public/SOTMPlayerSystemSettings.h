#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTMPlayerSystemSettings.generated.h"

UENUM(BlueprintType)
enum class ESOTMGameOverRetryPolicy : uint8
{
	RestoreAllLivesAtCheckpoint,
	RestoreOneLifeAtCheckpoint,
	Disabled
};

/**
 * Temporary, designer-editable defaults for the Chapter 1 player foundation.
 * Client-approved balance values can replace these without rewriting gameplay code.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTM Player System"))
class SOTM1_API USOTMPlayerSystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USOTMPlayerSystemSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Vitals", meta=(ClampMin="1.0"))
	float MaximumHealth = 100.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Lives", meta=(ClampMin="1"))
	int32 StartingLives = 5;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Lives", meta=(ClampMin="1"))
	int32 MaximumLives = 5;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float DamageInvulnerabilitySeconds = 1.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="0.0"))
	float RespawnDelaySeconds = 2.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Respawn")
	bool bRespawnWithFullHealth = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="!bRespawnWithFullHealth"))
	float RespawnHealthFraction = 1.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="0.0"))
	float RespawnSafetyInvulnerabilitySeconds = 1.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Game Over")
	ESOTMGameOverRetryPolicy RetryPolicy = ESOTMGameOverRetryPolicy::RestoreAllLivesAtCheckpoint;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Save")
	bool bAutoSaveOnCheckpoint = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Save")
	bool bAutoSaveOnDeath = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Save")
	bool bAutoLoadPlayerState = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Save")
	FString FallbackSaveSlotName = TEXT("Slot 1");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Routing")
	FName MainMenuMap = TEXT("/Game/Main_Menu_Map");
};
