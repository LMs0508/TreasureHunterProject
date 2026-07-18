// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RoundTimeData.h"
#include "TreasureHuntGameState.h"
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