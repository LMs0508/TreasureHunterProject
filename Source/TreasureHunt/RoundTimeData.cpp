#include "RoundTimeData.h"

const FRoundTimeEntry& URoundTimeData::GetRoundEntry(int32 RoundNumber) const
{
    for (const FRoundTimeEntry& Entry : Rounds)
    {
        if (Entry.Round == RoundNumber)
        {
            return Entry;
        }
    }

    // 정의된 라운드를 넘어가면 마지막 라운드 사용
    // 예: 5라운드까지 정의했는데 6라운드 진행 → 5라운드 시간 적용
    if (Rounds.Num() > 0)
    {
        return Rounds.Last();
    }

    static FRoundTimeEntry Default;
    return Default;
}