// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RoundTimeData.h"
#include "TreasureHuntGameState.h"
#include "StatueActor.h"
#include "TreasureHuntGameMode.generated.h"

UCLASS(minimalapi)
class ATreasureHuntGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATreasureHuntGameMode();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "Survival")
    URoundTimeData* RoundTimeData;

    FTimerHandle PhaseTimerHandle;

    // ── 석상 시스템 ─────────────────────────────
    // 스폰할 석상 개수 (후보 지점 중 이만큼 선택)
    UPROPERTY(EditDefaultsOnly, Category = "Statue")
    int32 StatueSpawnCount = 25;

    // 스폰할 석상 블루프린트 클래스 (BP_Statue 지정)
    UPROPERTY(EditDefaultsOnly, Category = "Statue")
    TSubclassOf<AStatueActor> StatueClass;

    // 스폰된 석상들 (서버 전용 관리)
    UPROPERTY()
    TArray<AStatueActor*> SpawnedStatues;

    // 현재 매치의 비밀번호 (자리순 0,1,2). 서버 전용
    TArray<int32> CurrentPassword;

    // 매치 시작 시: 후보 지점 중 골라 석상 스폰
    void SpawnStatues();

    // 첫날 밤: 숫자 3개 배치 (완전 고정이므로 1회만)
    void AssignPassword();
    // ────────────────────────────────────────────

    // GameState의 OnPhaseChanged 이벤트 핸들러
    // 라운드 시작 시 모든 캐릭터의 인벤토리 만료 처리 호출
    UFUNCTION()
    void OnPhaseChangedHandler(EGamePhase NewPhase, int32 NewRound);

    UFUNCTION()
    void StartDayPhase();

    UFUNCTION()
    void StartNightPhase();

    UFUNCTION()
    void OnNightPhaseEnd();


};