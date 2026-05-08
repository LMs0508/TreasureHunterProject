// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTimeData.generated.h"

USTRUCT(BlueprintType)
struct FRoundTimeEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Round = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "10.0"))
    float DaySeconds = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "10.0"))
    float NightSeconds = 120.f;
};


UCLASS(BlueprintType)
class TREASUREHUNT_API URoundTimeData : public UDataAsset
{
	GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FRoundTimeEntry> Rounds;

    // 라운드 번호로 항목 가져오기. 정의된 라운드를 넘으면 마지막 라운드 사용.
    const FRoundTimeEntry& GetRoundEntry(int32 RoundNumber) const;
};
