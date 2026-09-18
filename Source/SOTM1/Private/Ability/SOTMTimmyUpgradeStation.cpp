#include "Ability/SOTMTimmyUpgradeStation.h"

#include "Ability/SOTMPhase3Settings.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"

ASOTMTimmyUpgradeStation::ASOTMTimmyUpgradeStation()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StationRoot"));
	SetRootComponent(SceneRoot);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionSphere->SetGenerateOverlapEvents(true);

	StationTitle = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StationTitle"));
	StationTitle->SetupAttachment(SceneRoot);
	StationTitle->SetRelativeLocation(FVector(0.0f, 0.0f, 235.0f));
	StationTitle->SetHorizontalAlignment(EHTA_Center);
	StationTitle->SetVerticalAlignment(EVRTA_TextCenter);
	StationTitle->SetWorldSize(28.0f);
	StationTitle->SetText(FText::FromString(TEXT("TIMMY'S UPGRADE STATION")));
	StationTitle->SetTextRenderColor(FColor(186, 78, 255));
	StationTitle->SetCastShadow(false);
}

void ASOTMTimmyUpgradeStation::BeginPlay()
{
	Super::BeginPlay();
	InteractionSphere->SetSphereRadius(
		GetDefault<USOTMPhase3Settings>()->StationInteractionRadius,
		true);
	InteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleEndOverlap);
}

void ASOTMTimmyUpgradeStation::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;
	OnPlayerEntered.Broadcast(OtherActor);
}

void ASOTMTimmyUpgradeStation::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;
	OnPlayerExited.Broadcast(OtherActor);
}
