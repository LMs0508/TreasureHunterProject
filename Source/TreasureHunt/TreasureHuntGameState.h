#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TreasureHuntGameState.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Day   UMETA(DisplayName = "Day"),
    Night UMETA(DisplayName = "Night")
};

// 페이즈 변경 이벤트 디스패처
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhaseChangedSignature, EGamePhase, NewPhase, int32, NewRound);


UCLASS()
class TREASUREHUNT_API ATreasureHuntGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ATreasureHuntGameState();

    // 현재 페이즈 (낮/밤). 서버가 변경, 모든 클라가 읽음.
    UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "Phase")
    EGamePhase CurrentPhase = EGamePhase::Day;

    // 현재 라운드 번호 (1부터 시작).
    UPROPERTY(ReplicatedUsing = OnRep_Round, BlueprintReadOnly, Category = "Phase")
    int32 CurrentRound = 1;

    // 현재 페이즈의 남은 시간 (초). 서버에서 매 틱 감소.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Phase")
    float PhaseTimeRemaining = 0.f;

    // 페이즈가 변경될 때마다 발행되는 이벤트 (모든 구독자에게 알림)
    UPROPERTY(BlueprintAssignable, Category = "Phase")
    FOnPhaseChangedSignature OnPhaseChanged;

    // 서버 전용. GameMode가 페이즈 전환 시 호출.
    void Server_StartPhase(EGamePhase NewPhase, int32 NewRound, float Duration);

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION()
    void OnRep_Phase();

    UFUNCTION()
    void OnRep_Round();
};