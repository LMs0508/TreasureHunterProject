#include "InventoryComponent.h"
#include "ItemData.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 컴포넌트 자체를 Replicate
    SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UInventoryComponent, Items);
}

void UInventoryComponent::OnRep_Items()
{
    // 클라이언트 측에서 배열이 갱신됐을 때 UI 등에 알림
    OnInventoryChanged.Broadcast();
}

// ===== Server 함수 =====

bool UInventoryComponent::Server_TryAddItem(UItemData* Item, int32 Count)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Inventory] Server_TryAddItem called without authority"));
        return false;
    }

    if (!Item || Count <= 0)
    {
        return false;
    }

    int32 Remaining = Count;

    // 1단계: 스택 가능한 기존 슬롯에 추가
    Remaining = TryStackToExisting(Item, Remaining);
    if (Remaining == 0)
    {
        OnInventoryChanged.Broadcast();  // 서버 자신도 알림
        return true;
    }

    // 2단계: 새 슬롯에 추가
    Remaining = TryAddToNewSlot(Item, Remaining);

    // 서버 자신의 OnRep은 자동 호출 안 되므로 직접 알림
    OnInventoryChanged.Broadcast();

    // Remaining이 0이면 다 들어감, > 0이면 일부만 들어감 (실패로 간주)
    return Remaining == 0;
}

bool UInventoryComponent::Server_RemoveItem(int32 EntryIndex, int32 Count)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    if (!Items.IsValidIndex(EntryIndex) || Count <= 0)
    {
        return false;
    }

    FInventoryEntry& Entry = Items[EntryIndex];
    if (Entry.Count < Count)
    {
        return false;  // 가지고 있는 것보다 많이 빼려고 함
    }

    Entry.Count -= Count;
    if (Entry.Count <= 0)
    {
        Items.RemoveAt(EntryIndex);
    }

    OnInventoryChanged.Broadcast();
    return true;
}

void UInventoryComponent::Server_UpdateItemExpiry()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    bool bChanged = false;

    // 역순 순회로 안전하게 제거
    for (int32 i = Items.Num() - 1; i >= 0; --i)
    {
        if (!Items[i].Item)
        {
            continue;
        }

        // 카운터 증가
        Items[i].RoundsSinceCreated++;

        // RoundsToExpire가 0이면 영구 (재료, 레시피)
        // 0이 아니고 카운터가 초과했으면 제거
        const int32 Limit = Items[i].Item->RoundsToExpire;
        if (Limit > 0 && Items[i].RoundsSinceCreated >= Limit)
        {
            UE_LOG(LogTemp, Log, TEXT("[Inventory] Item %s expired (rounds: %d/%d)"),
                *Items[i].Item->DisplayName.ToString(),
                Items[i].RoundsSinceCreated,
                Limit);
            Items.RemoveAt(i);
            bChanged = true;
        }
    }

    if (bChanged)
    {
        OnInventoryChanged.Broadcast();
    }
}

// ===== 조회 함수 =====

int32 UInventoryComponent::GetUsedSlots() const
{
    int32 Used = 0;
    for (const FInventoryEntry& Entry : Items)
    {
        if (Entry.Item)
        {
            // 그리드 방식 (기획서 v1.2): 스택 하나가 자기 모양만큼 칸 차지
            Used += Entry.Item->GridWidth * Entry.Item->GridHeight;
        }
    }
    return Used;
}

int32 UInventoryComponent::GetRemainingSlots() const
{
    return FMath::Max(0, (GridColumns * GridRows) - GetUsedSlots());
}

int32 UInventoryComponent::GetItemCount(UItemData* Item) const
{
    if (!Item)
    {
        return 0;
    }

    int32 Total = 0;
    for (const FInventoryEntry& Entry : Items)
    {
        if (Entry.Item == Item)
        {
            Total += Entry.Count;
        }
    }
    return Total;
}

// ===== 내부 헬퍼 =====

int32 UInventoryComponent::TryStackToExisting(UItemData* Item, int32 Count)
{
    if (Item->MaxStackSize <= 1)
    {
        // 스택 불가 아이템이면 기존 슬롯 못 씀
        return Count;
    }

    int32 Remaining = Count;
    for (FInventoryEntry& Entry : Items)
    {
        if (Remaining <= 0) break;

        if (Entry.Item == Item && Entry.Count < Item->MaxStackSize)
        {
            int32 CanAdd = Item->MaxStackSize - Entry.Count;
            int32 ToAdd = FMath::Min(Remaining, CanAdd);
            Entry.Count += ToAdd;
            Remaining -= ToAdd;
        }
    }
    return Remaining;
}

int32 UInventoryComponent::TryAddToNewSlot(UItemData* Item, int32 Count)
{
    int32 Remaining = Count;
    while (Remaining > 0)
    {
        // 그리드에서 이 아이템 모양이 들어갈 자리 탐색
        int32 FoundX = 0;
        int32 FoundY = 0;
        if (!FindFreeGridPosition(Item->GridWidth, Item->GridHeight, FoundX, FoundY))
        {
            break;  // 자리 없음 → 남은 개수 그대로 반환 (부분 실패)
        }

        FInventoryEntry NewEntry;
        NewEntry.Item = Item;
        NewEntry.Count = FMath::Min(Remaining, Item->MaxStackSize);
        NewEntry.GridX = FoundX;
        NewEntry.GridY = FoundY;
        Items.Add(NewEntry);
        Remaining -= NewEntry.Count;

        UE_LOG(LogTemp, Log, TEXT("[Inventory] Placed %s at (%d, %d) size %dx%d"),
            *Item->ItemID.ToString(), FoundX, FoundY, Item->GridWidth, Item->GridHeight);
    }
    return Remaining;
}

bool UInventoryComponent::FindFreeGridPosition(int32 ItemWidth, int32 ItemHeight, int32& OutX, int32& OutY) const
{
    // 1) 점유 맵 만들기: 20칸을 전부 "비어있음(false)"으로 시작
    bool bOccupied[GridColumns * GridRows] = { false };

    // 기존 아이템들이 차지한 칸을 전부 true로 칠한다
    for (const FInventoryEntry& Entry : Items)
    {
        if (!Entry.Item) continue;

        for (int32 Y = Entry.GridY; Y < Entry.GridY + Entry.Item->GridHeight; ++Y)
        {
            for (int32 X = Entry.GridX; X < Entry.GridX + Entry.Item->GridWidth; ++X)
            {
                if (X >= 0 && X < GridColumns && Y >= 0 && Y < GridRows)
                {
                    bOccupied[Y * GridColumns + X] = true;
                }
            }
        }
    }

    // 2) 좌상단부터 스캔: 각 위치를 "후보 좌상단"으로 삼아 검사
    //    (아이템이 그리드 밖으로 삐져나가지 않는 범위까지만 후보로)
    for (int32 Y = 0; Y <= GridRows - ItemHeight; ++Y)
    {
        for (int32 X = 0; X <= GridColumns - ItemWidth; ++X)
        {
            // 이 위치에 놓았을 때 모양이 덮는 칸들이 전부 비어있는지 검사
            bool bFits = true;
            for (int32 DY = 0; DY < ItemHeight && bFits; ++DY)
            {
                for (int32 DX = 0; DX < ItemWidth && bFits; ++DX)
                {
                    if (bOccupied[(Y + DY) * GridColumns + (X + DX)])
                    {
                        bFits = false;
                    }
                }
            }

            if (bFits)
            {
                OutX = X;
                OutY = Y;
                return true;  // 첫 번째로 맞는 자리 발견
            }
        }
    }

    return false;  // 그리드 어디에도 자리 없음
}