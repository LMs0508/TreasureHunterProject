// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RoundTimeData.h"
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

    UFUNCTION()
    void StartDayPhase();

    UFUNCTION()
    void StartNightPhase();

    UFUNCTION()
    void OnNightPhaseEnd();
};