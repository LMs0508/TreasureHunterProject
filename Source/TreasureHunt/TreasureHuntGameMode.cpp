// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreasureHuntGameMode.h"
#include "TreasureHuntCharacter.h"
#include "TreasureHuntGameState.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "InventoryComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"


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

    ATreasureHuntGameState* GS = Cast<ATreasureHuntGameState>(GameState);
    if (GS)
    {
        GS->OnPhaseChanged.AddDynamic(this, &ATreasureHuntGameMode::OnPhaseChangedHandler);
    }
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

void ATreasureHuntGameMode::OnPhaseChangedHandler(EGamePhase NewPhase, int32 NewRound)
{
    // Day로 전환된 시점에 인벤토리 만료 처리
    // 단, 첫 번째 라운드의 첫 Day는 제외 (아직 정리할 게 없음)
    if (NewPhase == EGamePhase::Day && NewRound > 1)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Round %d 시작. 인벤토리 만료 처리 중..."), NewRound);

        // 모든 캐릭터 찾기
        TArray<AActor*> AllCharacters;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllCharacters);

        // 각 캐릭터의 InventoryComponent에 만료 처리 호출
        for (AActor* Actor : AllCharacters)
        {
            if (UInventoryComponent* InvComp = Actor->FindComponentByClass<UInventoryComponent>())
            {
                InvComp->Server_UpdateItemExpiry();
            }
        }
    }
}