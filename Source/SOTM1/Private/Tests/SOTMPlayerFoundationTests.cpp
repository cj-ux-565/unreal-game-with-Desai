#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "SOTMPlayerStateSubsystem.h"
#include "SOTMPlayerVitalComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSOTMPlayerVitalsTest,
	"SOTM.PlayerFoundation.Vitals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSOTMPlayerVitalsTest::RunTest(const FString& Parameters)
{
	USOTMPlayerVitalComponent* Vitals = NewObject<USOTMPlayerVitalComponent>();
	Vitals->InitializeVitals(100.0f, 100.0f);

	TestEqual(TEXT("Maximum health"), Vitals->GetMaximumHealth(), 100.0f);
	TestEqual(TEXT("Starting health"), Vitals->GetCurrentHealth(), 100.0f);

	TestTrue(TEXT("Partial damage accepted"), Vitals->ApplySOTMDamage(25.0f));
	TestEqual(TEXT("Partial damage applied once"), Vitals->GetCurrentHealth(), 75.0f);

	TestFalse(TEXT("Damage ignored during invulnerability"), Vitals->ApplySOTMDamage(25.0f));
	TestEqual(TEXT("Invulnerable health unchanged"), Vitals->GetCurrentHealth(), 75.0f);

	Vitals->SetInvulnerable(false);
	TestEqual(TEXT("Healing clamps to maximum"), Vitals->Heal(1000.0f), 25.0f);
	TestEqual(TEXT("Health does not exceed maximum"), Vitals->GetCurrentHealth(), 100.0f);

	Vitals->SetInvulnerable(false);
	TestTrue(TEXT("Fatal damage accepted"), Vitals->ApplySOTMDamage(100.0f));
	TestTrue(TEXT("Fatal damage marks dead"), Vitals->IsDead());
	TestEqual(TEXT("Fatal damage reaches zero"), Vitals->GetCurrentHealth(), 0.0f);
	TestFalse(TEXT("Repeated fatal damage is ignored"), Vitals->ApplySOTMDamage(100.0f));

	Vitals->Revive(100.0f);
	TestFalse(TEXT("Revive clears death"), Vitals->IsDead());
	TestEqual(TEXT("Revive restores configured health"), Vitals->GetCurrentHealth(), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSOTMInputLockReferenceCountTest,
	"SOTM.PlayerFoundation.InputLocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSOTMInputLockReferenceCountTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USOTMPlayerStateSubsystem* State = NewObject<USOTMPlayerStateSubsystem>(GameInstance);
	State->AcquireInputLock(ESOTMInputLockReason::Cinematic);
	State->AcquireInputLock(ESOTMInputLockReason::Cinematic);
	State->AcquireInputLock(ESOTMInputLockReason::JumpScare);

	TestEqual(TEXT("Same reason is reference counted"), State->GetInputLockCount(ESOTMInputLockReason::Cinematic), 2);
	TestTrue(TEXT("Multiple reasons hold the lock"), State->HasAnyInputLock());

	State->ReleaseInputLock(ESOTMInputLockReason::Cinematic);
	TestEqual(TEXT("One release does not clear a double lock"), State->GetInputLockCount(ESOTMInputLockReason::Cinematic), 1);
	State->ClearInputLock(ESOTMInputLockReason::JumpScare);
	TestTrue(TEXT("Remaining reason still locks input"), State->HasAnyInputLock());
	State->ReleaseInputLock(ESOTMInputLockReason::Cinematic);
	TestFalse(TEXT("Final matching release clears input"), State->HasAnyInputLock());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSOTMSaveSchemaTest,
	"SOTM.PlayerFoundation.SaveSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSOTMSaveSchemaTest::RunTest(const FString& Parameters)
{
	UClass* SaveClass = LoadClass<UObject>(
		nullptr,
		TEXT("/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject.BP_CustomSaveGameObject_C"));
	TestNotNull(TEXT("Production custom SaveGame class loads"), SaveClass);
	if (!SaveClass)
	{
		return false;
	}

	const TArray<FName> RequiredFields = {
		TEXT("SOTM_HasPlayerState"),
		TEXT("SOTM_SaveVersion"),
		TEXT("SOTM_CurrentLives"),
		TEXT("SOTM_MaximumLives"),
		TEXT("SOTM_CurrentHealth"),
		TEXT("SOTM_CheckpointId"),
		TEXT("SOTM_CheckpointMap"),
		TEXT("SOTM_CheckpointTransform"),
		TEXT("SOTM_AvailableCoins"),
		TEXT("SOTM_LifetimeCoinsCollected"),
		TEXT("SOTM_CollectedCoinIds")
	};

	for (const FName Field : RequiredFields)
	{
		TestNotNull(*FString::Printf(TEXT("Save field %s exists"), *Field.ToString()), FindFProperty<FProperty>(SaveClass, Field));
	}
	return true;
}

#endif
