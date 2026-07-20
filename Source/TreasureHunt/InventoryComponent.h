// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemData;

USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemData> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Count = 0;

	// 그리드 좌상단 위치 (기획서 v1.2: 5x4 그리드, 0부터 시작)
	// GridX: 0~4 (열), GridY: 0~3 (행)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 GridX = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 GridY = 0;

	// 이 아이템이 인벤토리에 담긴 후 지난 라운드 수
	// 0 = 이번 라운드에 들어옴
	// 라운드 종료 시 +1 됨
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RoundsSinceCreated = 0;
};

// 인벤토리 변경 이벤트 (UI 갱신 등에 사용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TREASUREHUNT_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	UInventoryComponent();

	// 인벤토리 최대 슬롯 수 (기획서: 10칸 고정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory",
		meta = (ClampMin = "1", ClampMax = "30"))
	int32 MaxSlots = 10;

	// ===== 그리드 설정 (기획서 v1.2: 5x4 = 20칸) =====
	static constexpr int32 GridColumns = 5;
	static constexpr int32 GridRows = 4;

	// 인벤토리 내용 (Replicated, 모든 클라가 봄)
	UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryEntry> Items;

	// 인벤토리 변경 시 발행되는 이벤트 (UI 구독용)
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChangedSignature OnInventoryChanged;

	// ===== 서버 권한 함수들 =====
	// 아이템 추가 시도. 성공 시 true, 공간 부족 등으로 실패 시 false.
	// 스택 가능한 아이템은 기존 슬롯에 누적, 불가능하면 새 슬롯에.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Server_TryAddItem(UItemData* Item, int32 Count);

	// 특정 슬롯에서 아이템 제거.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Server_RemoveItem(int32 EntryIndex, int32 Count);

	// 라운드 종료 시 호출.
	// 모든 아이템의 RoundsSinceCreated를 +1하고,
	// RoundsToExpire를 초과한 아이템 제거.
	// RoundsToExpire = 0인 아이템(재료, 레시피)은 영구 유지.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Server_UpdateItemExpiry();

	// ===== 조회 함수들 (서버/클라 모두 호출 가능) =====
	// 현재 사용 중인 슬롯 수 합계
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetUsedSlots() const;

	// 남은 슬롯 수
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetRemainingSlots() const;

	// 특정 아이템이 인벤토리에 몇 개 있는지
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(UItemData* Item) const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Items 배열이 클라이언트에 동기화되었을 때 호출됨
	UFUNCTION()
	void OnRep_Items();

	//virtual void BeginPlay() override;

private:
	// 내부 헬퍼: 스택 가능한 기존 슬롯에 추가 시도
	// 반환: 남은 추가할 개수 (0이면 다 들어감)
	int32 TryStackToExisting(UItemData* Item, int32 Count);

	// 내부 헬퍼: 새 슬롯에 추가 시도
	// 반환: 남은 추가할 개수
	int32 TryAddToNewSlot(UItemData* Item, int32 Count);

	// 내부 헬퍼: 그리드에서 ItemWidth x ItemHeight 모양이 들어갈 첫 빈 자리 탐색
	// 성공 시 true + OutX/OutY에 좌상단 좌표, 자리 없으면 false
	bool FindFreeGridPosition(int32 ItemWidth, int32 ItemHeight, int32& OutX, int32& OutY) const;
};