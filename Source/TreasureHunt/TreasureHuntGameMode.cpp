// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreasureHuntGameMode.h"
#include "TreasureHuntCharacter.h"
#include "TreasureHuntGameState.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATreasureHuntGameMode::ATreasureHuntGameMode()
    : Super()
{
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
    DefaultPawnClass = PlayerPawnClassFinder.Class;

    GameStateClass = ATreasureHuntGameState::StaticClass();
}

void ATreasureHuntGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    if (!RoundTimeData)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] RoundTimeData not assigned"));
        return;
    }

    if (RoundTimeData->Rounds.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] DA_RoundTimes is empty"));
        return;
    }

    StartDayPhase();
}

void ATreasureHuntGameMode::StartDayPhase()
{
    ATreasureHuntGameState* GS = Cast<ATreasureHuntGameState>(GameState);
    if (GS == nullptr)
    {
        return;
    }
    if (RoundTimeData == nullptr)
    {
        return;
    }

    int32 RoundNum = GS->CurrentRound;
    FRoundTimeEntry Entry = RoundTimeData->GetRoundEntry(RoundNum);

    GS->Server_StartPhase(EGamePhase::Day, RoundNum, Entry.DaySeconds);

    GetWorldTimerManager().SetTimer(
        PhaseTimerHandle,
        this,
        &ATreasureHuntGameMode::StartNightPhase,
        Entry.DaySeconds,
        false
    );
}

void ATreasureHuntGameMode::StartNightPhase()
{
    ATreasureHuntGameState* GS = Cast<ATreasureHuntGameState>(GameState);
    if (GS == nullptr)
    {
        return;
    }
    if (RoundTimeData == nullptr)
    {
        return;
    }

    int32 RoundNum = GS->CurrentRound;
    FRoundTimeEntry Entry = RoundTimeData->GetRoundEntry(RoundNum);

    GS->Server_StartPhase(EGamePhase::Night, RoundNum, Entry.NightSeconds);

    GetWorldTimerManager().SetTimer(
        PhaseTimerHandle,
        this,
        &ATreasureHuntGameMode::OnNightPhaseEnd,
        Entry.NightSeconds,
        false
    );
}

void ATreasureHuntGameMode::OnNightPhaseEnd()
{
    ATreasureHuntGameState* GS = Cast<ATreasureHuntGameState>(GameState);
    if (GS != nullptr)
    {
        GS->CurrentRound += 1;
    }

    StartDayPhase();
}