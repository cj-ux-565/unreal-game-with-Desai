#include "AI/SOTMIsabelPatrolPoint.h"

#include "Components/SceneComponent.h"

ASOTMIsabelPatrolPoint::ASOTMIsabelPatrolPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = true;
#endif
}
