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

void UInventoryComponent::Server_ClearNonSurvivingItems()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    // bSurvivesRoundEnd가 false인 아이템 제거 (역순 순회로 안전하게)
    bool bChanged = false;
    for (int32 i = Items.Num() - 1; i >= 0; --i)
    {
        if (Items[i].Item && !Items[i].Item->bSurvivesRoundEnd)
        {
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
            // 칸 합산 방식: SlotsRequired × 스택 수가 아니라, 스택 자체가 한 슬롯 묶음
            // 기획서 §5.1: 슬롯 합산 (Tetris식 아님)
            // → 한 슬롯에 99개 쌓여도 SlotsRequired만 차지
            Used += Entry.Item->SlotsRequired;
        }
    }
    return Used;
}

int32 UInventoryComponent::GetRemainingSlots() const
{
    return FMath::Max(0, MaxSlots - GetUsedSlots());
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
    while (Remaining > 0 && GetRemainingSlots() >= Item->SlotsRequired)
    {
        FInventoryEntry NewEntry;
        NewEntry.Item = Item;
        NewEntry.Count = FMath::Min(Remaining, Item->MaxStackSize);
        Items.Add(NewEntry);
        Remaining -= NewEntry.Count;
    }
    return Remaining;
}