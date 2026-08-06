#include "StatueSpawnPoint.h"
#include "Components/BillboardComponent.h"

AStatueSpawnPoint::AStatueSpawnPoint()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

#if WITH_EDITORONLY_DATA
    Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    Billboard->SetupAttachment(Root);
#endif
}