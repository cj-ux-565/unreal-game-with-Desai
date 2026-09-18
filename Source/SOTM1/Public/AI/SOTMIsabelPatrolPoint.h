#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTMIsabelPatrolPoint.generated.h"

class USceneComponent;

/** An authored, reusable point in an Isabel patrol route. Contains no Tick or gameplay logic. */
UCLASS(BlueprintType)
class SOTM1_API ASOTMIsabelPatrolPoint : public AActor
{
	GENERATED_BODY()

public:
	ASOTMIsabelPatrolPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Patrol")
	FName RouteId = TEXT("Isabel_Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Patrol")
	int32 Order = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTM|Isabel|Patrol", meta=(ClampMin="0.0"))
	float WaitDuration = 1.5f;

private:
	UPROPERTY(VisibleAnywhere, Category="SOTM|Isabel|Patrol")
	TObjectPtr<USceneComponent> SceneRoot;
};
