#include "Demo/SOTMPhase4Interactable.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"

ASOTMPhase4Interactable::ASOTMPhase4Interactable()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetSphereRadius(425.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleEndOverlap);
}

void ASOTMPhase4Interactable::Configure(const ESOTMPhase4InteractableKind InKind, const float Radius)
{
	Kind = InKind;
	InteractionSphere->SetSphereRadius(FMath::Max(100.0f, Radius), true);
}

void ASOTMPhase4Interactable::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;
	OnPlayerEntered.Broadcast(Kind, OtherActor);
}

void ASOTMPhase4Interactable::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const int32 OtherBodyIndex)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;
	OnPlayerExited.Broadcast(Kind, OtherActor);
}
