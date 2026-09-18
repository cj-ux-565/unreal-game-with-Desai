#include "AI/SOTMIsabelJumpScareImpactNotify.h"

#include "AI/SOTMIsabelAIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"

void USOTMIsabelJumpScareImpactNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const APawn* IsabelPawn = MeshComp ? Cast<APawn>(MeshComp->GetOwner()) : nullptr;
	if (ASOTMIsabelAIController* IsabelController = IsabelPawn
		? Cast<ASOTMIsabelAIController>(IsabelPawn->GetController())
		: nullptr)
	{
		IsabelController->HandleJumpScareImpactNotify();
	}
}

FString USOTMIsabelJumpScareImpactNotify::GetNotifyName_Implementation() const
{
	return TEXT("Isabel Jump Scare Presentation Impact");
}
