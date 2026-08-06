// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreasureHuntGameMode.h"
#include "TreasureHuntCharacter.h"
#include "TreasureHuntGameState.h"
#include "StatueActor.h"
#include "StatueSpawnPoint.h"
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
	// 석상 스폰
    SpawnStatues();

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
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Round %d Start... Inventory Cleaning..."), NewRound);

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
    // 첫날밤 비밀번호 배치
    if (NewPhase == EGamePhase::Night && NewRound == 1)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] First Night... Password Patching...."));
        AssignPassword();
    }
}


void ATreasureHuntGameMode::SpawnStatues()
{
    if (!HasAuthority())
    {
        return;
    }

    if (StatueClass == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] StatueClass is not selected"));
        return;
    }

    // 1. 레벨의 모든 스폰 후보 수집
    TArray<AActor*> FoundPoints;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), AStatueSpawnPoint::StaticClass(), FoundPoints);

    if (FoundPoints.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] StatueSpawnPoint is Not in Level"));
        return;
    }

    // 2. 셔플 (Fisher-Yates)
    for (int32 i = FoundPoints.Num() - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        FoundPoints.Swap(i, j);
    }

    // 3. 앞에서부터 StatueSpawnCount개만 스폰
    const int32 SpawnNum = FMath::Min(StatueSpawnCount, FoundPoints.Num());
    for (int32 i = 0; i < SpawnNum; ++i)
    {
        AStatueSpawnPoint* Point = Cast<AStatueSpawnPoint>(FoundPoints[i]);
        if (Point == nullptr)
        {
            continue;
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AStatueActor* NewStatue = GetWorld()->SpawnActor<AStatueActor>(
            StatueClass,
            Point->GetActorLocation(),
            Point->GetActorRotation(),
            Params);

        if (NewStatue != nullptr)
        {
            NewStatue->RegionID = Point->RegionID;
            SpawnedStatues.Add(NewStatue);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode] Spawned % d statues"), SpawnedStatues.Num());
}

void ATreasureHuntGameMode::AssignPassword()
{
    if (!HasAuthority())
    {
        return;
    }

    // 1. 전체 초기화
    for (int32 i = 0; i < SpawnedStatues.Num(); ++i)
    {
        if (SpawnedStatues[i] != nullptr)
        {
            SpawnedStatues[i]->ClearNumber();
        }
    }

    // 2. 석상 인덱스 셔플
    TArray<int32> Indices;
    for (int32 i = 0; i < SpawnedStatues.Num(); ++i)
    {
        Indices.Add(i);
    }
    for (int32 i = Indices.Num() - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        Indices.Swap(i, j);
    }

    // 3. 중복 없는 숫자 3개 (0~9 셔플 후 앞 3개)
    TArray<int32> Digits;
    for (int32 d = 0; d <= 9; ++d)
    {
        Digits.Add(d);
    }
    for (int32 i = Digits.Num() - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        Digits.Swap(i, j);
    }

    // 4. 할당
    CurrentPassword.Empty();
    const int32 AssignNum = FMath::Min(3, Indices.Num());
    for (int32 k = 0; k < AssignNum; ++k)
    {
        AStatueActor* Target = SpawnedStatues[Indices[k]];
        if (Target != nullptr)
        {
            Target->InitStatue(Digits[k], k);
            CurrentPassword.Add(Digits[k]);
        }
    }

    // 서버 로그로 확인 (테스트용 - 나중에 제거)
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] PassWord fetching: %d-%d-%d"),
        CurrentPassword.IsValidIndex(0) ? CurrentPassword[0] : -1,
        CurrentPassword.IsValidIndex(1) ? CurrentPassword[1] : -1,
        CurrentPassword.IsValidIndex(2) ? CurrentPassword[2] : -1);
}