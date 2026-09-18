#include "AI/SOTMIsabelAttackImpactNotify.h"

#include "AI/SOTMIsabelAIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"

void USOTMIsabelAttackImpactNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const APawn* IsabelPawn = MeshComp ? Cast<APawn>(MeshComp->GetOwner()) : nullptr;
	if (ASOTMIsabelAIController* IsabelController = IsabelPawn ? Cast<ASOTMIsabelAIController>(IsabelPawn->GetController()) : nullptr)
	{
		IsabelController->HandleAttackImpactNotify();
	}
}

FString USOTMIsabelAttackImpactNotify::GetNotifyName_Implementation() const
{
	return TEXT("Isabel Attack Impact");
}
