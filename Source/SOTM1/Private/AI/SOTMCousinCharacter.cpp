#include "AI/SOTMCousinCharacter.h"

#include "AI/SOTMCousinAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ASOTMCousinCharacter::ASOTMCousinCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(TEXT("SOTM_Cousin"));
	AIControllerClass = ASOTMCousinAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(32.0f, 72.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -72.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DollMesh(
		TEXT("/Game/AI/CruelDoll/Meshes/SK_CruelDoll.SK_CruelDoll"));
	if (DollMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(DollMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> DollAnimBlueprint(
		TEXT("/Game/AI/CruelDoll/Meshes/SKL_CruelDoll_AnimBlueprint"));
	if (DollAnimBlueprint.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(DollAnimBlueprint.Class);
	}

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 190.0f;
	GetCharacterMovement()->MaxAcceleration = 900.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 900.0f;
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceWeight = 0.45f;
}

void ASOTMCousinCharacter::SetPresentationVariant(const int32 InVariantIndex)
{
	PresentationVariant = FMath::Abs(InVariantIndex) % 3;
	SetActorScale3D(PresentationVariant == 0
		? FVector(0.92f)
		: (PresentationVariant == 1 ? FVector(1.0f) : FVector(0.86f)));
}
