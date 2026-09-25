#include "Combat/SOTMIsabelDamageProjectile.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

namespace SOTMIsabelProjectilePrivate
{
	// Existing lightning visual already used by BP_LightingThrow. Referenced, not created.
	const TCHAR* LightningParticlePath =
		TEXT("/Game/SuperPowers/Powers/Speedster/Rays/Particles/P_Sphere_of_Lightning.P_Sphere_of_Lightning");
}

ASOTMIsabelDamageProjectile::ASOTMIsabelDamageProjectile()
{
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(24.0f);
	RootComponent = CollisionSphere;
	// Overlap-based damage: overlap Pawns, block world geometry for sweeps.
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleBeginOverlap);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = FlightSpeed;
	ProjectileMovement->MaxSpeed = FlightSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bInitialVelocityInLocalSpace = true;

	// Reuse the existing lightning visual; projectile stays functional without it.
	LightningVisual = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LightningVisual"));
	LightningVisual->SetupAttachment(CollisionSphere);
	if (UParticleSystem* LightningTemplate = LoadObject<UParticleSystem>(nullptr, SOTMIsabelProjectilePrivate::LightningParticlePath))
	{
		LightningVisual->SetTemplate(LightningTemplate);
	}

	SetLifeSpan(LifeSeconds);
}

void ASOTMIsabelDamageProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (!ProjectileMovement)
	{
		return;
	}
	// Belt-and-braces init: proper registration path (tick + simulation),
	// then explicit velocity from the spawn forward direction. Direct Velocity
	// assignment is required because InitializeComponent only rescales an
	// already nonzero velocity and never creates one from InitialSpeed.
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->SetComponentTickEnabled(true);
	ProjectileMovement->Velocity = GetActorForwardVector() * FlightSpeed;
}

void ASOTMIsabelDamageProjectile::HandleBeginOverlap(
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

	if (!IsValid(OtherActor) || ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	UGameplayStatics::ApplyDamage(
		OtherActor, DamageAmount, GetInstigatorController(), this, UDamageType::StaticClass());
	Destroy();
}

bool ASOTMIsabelDamageProjectile::ShouldIgnoreActor(const AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == this)
	{
		return true;
	}
	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return true;
	}
	// The shooter is always the local player pawn in CH1; never damage it,
	// even if the Blueprint spawn left Owner/Instigator unset.
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (OtherActor == PC->GetPawn())
			{
				return true;
			}
		}
	}
	return false;
}
