// TreasureHuntGameState.cpp
#include "TreasureHuntGameState.h"
#include "Net/UnrealNetwork.h"

ATreasureHuntGameState::ATreasureHuntGameState()
{
    // Tick 활성화 (PhaseTimeRemaining 감소용)
    PrimaryActorTick.bCanEverTick = true;
}

void ATreasureHuntGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATreasureHuntGameState, CurrentPhase);
    DOREPLIFETIME(ATreasureHuntGameState, CurrentRound);
    DOREPLIFETIME(ATreasureHuntGameState, PhaseTimeRemaining);
}

void ATreasureHuntGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 서버에서만 시간 감소
    if (HasAuthority() && PhaseTimeRemaining > 0.f)
    {
        PhaseTimeRemaining -= DeltaSeconds;
        if (PhaseTimeRemaining < 0.f)
        {
            PhaseTimeRemaining = 0.f;
        }
    }
}

void ATreasureHuntGameState::Server_StartPhase(EGamePhase NewPhase, int32 NewRound, float Duration)
{
    // 안전장치: 서버에서만 실행
    if (!HasAuthority())
    {
        return;
    }

    CurrentPhase = NewPhase;
    CurrentRound = NewRound;
    PhaseTimeRemaining = Duration;

    // 서버 자신도 OnRep을 호출해야 일관성 유지 (서버 호스트도 클라 역할 함)
    OnRep_Phase();
    OnRep_Round();
}

void ATreasureHuntGameState::OnRep_Phase()
{
    UE_LOG(LogTemp, Log, TEXT("[Phase] %s 시작 (Round %d, %.0f초)"),
        CurrentPhase == EGamePhase::Day ? TEXT("Day") : TEXT("Night"),
        CurrentRound,
        PhaseTimeRemaining);

    // 다음 작업에서 라이팅/BGM 변경 이벤트 디스패처 호출 예정
}

void ATreasureHuntGameState::OnRep_Round()
{
    // 라운드 자체 변경 처리는 OnRep_Phase에서 함께 로그하므로 비워둠
    // 추후 라운드 변경 시 별도 효과(공지 등)가 필요하면 여기 추가
}