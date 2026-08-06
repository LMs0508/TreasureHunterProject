#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StatueSpawnPoint.generated.h"

UCLASS()
class TREASUREHUNT_API AStatueSpawnPoint : public AActor  // ← 모듈명 확인 필요
{
    GENERATED_BODY()

public:
    AStatueSpawnPoint();

    // 이 지점이 속한 지역 (0~4). 레벨에서 에디터로 지정
    UPROPERTY(EditAnywhere, Category = "Statue")
    int32 RegionID = 0;

#if WITH_EDITORONLY_DATA
    // 에디터에서만 보이는 표시용 컴포넌트 (게임에는 안 나옴)
    UPROPERTY(VisibleAnywhere)
    class UBillboardComponent* Billboard;
#endif
};