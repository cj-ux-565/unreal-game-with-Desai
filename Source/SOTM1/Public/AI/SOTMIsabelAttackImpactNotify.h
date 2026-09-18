#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "SOTMIsabelAttackImpactNotify.generated.h"

/** Phase 2 notify that delegates the impact to the possessing Isabel controller. */
UCLASS(meta=(DisplayName="SOTM Isabel Attack Impact"))
class SOTM1_API USOTMIsabelAttackImpactNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
