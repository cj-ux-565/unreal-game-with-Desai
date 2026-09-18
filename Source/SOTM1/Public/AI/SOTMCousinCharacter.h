#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SOTMCousinCharacter.generated.h"

/** Native reusable production Cousin pawn using the existing Cruel Doll assets. */
UCLASS(Blueprintable)
class SOTM1_API ASOTMCousinCharacter final : public ACharacter
{
	GENERATED_BODY()

public:
	ASOTMCousinCharacter();

	void SetPresentationVariant(int32 InVariantIndex);
	int32 GetPresentationVariant() const { return PresentationVariant; }

private:
	UPROPERTY(VisibleInstanceOnly, Category="SOTM|Cousin")
	int32 PresentationVariant = 0;
};

