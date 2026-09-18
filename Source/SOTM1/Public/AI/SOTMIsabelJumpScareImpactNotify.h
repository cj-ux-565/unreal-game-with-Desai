#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "SOTMIsabelJumpScareImpactNotify.generated.h"

/** Presentation-only timing notify for the catch jump scare; it never applies player damage. */
UCLASS(meta=(DisplayName="SOTM Isabel Jump Scare Presentation Impact"))
class SOTM1_API USOTMIsabelJumpScareImpactNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
