#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StatueActor.generated.h"

UCLASS()
class TREASUREHUNT_API AStatueActor : public AActor
{
    GENERATED_BODY()

public:
    AStatueActor();

    //서버에서만 호출. 숫자와 자릿수를 심는다
    void InitStatue(int32 InNumber, int32 InIndex);

    //숫자 제거 (숫자 없는 석상으로)
    void ClearNumber();

    bool HasNumber() const { return SecretNumber >= 0; }

    //이 석상이 속한 지역 (스폰 지점에서 복사됨)
    int32 RegionID = 0;

protected:
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere)
    class UStaticMeshComponent* Mesh;

    //Replicated 절대 금지 — 서버에만 존재하는 정답
    int32 SecretNumber = -1;    // -1 = 숫자 없음

    //비밀번호에서 몇 번째 자리인지 (0,1,2). 순서 추리용
    int32 PasswordIndex = -1;

    //첫날 밤에 상호작용 가능해짐. 이건 복제 O
    UPROPERTY(ReplicatedUsing = OnRep_Activated)
    bool bActivated = false;

    UFUNCTION()
    void OnRep_Activated();

public:
    //서버에서 밤 진입 시 호출
    void SetActivated(bool bNewActivated);
};