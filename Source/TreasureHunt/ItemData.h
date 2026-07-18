#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemData.generated.h"

// 아이템 종류
UENUM(BlueprintType)
enum class EItemType : uint8
{
    Material   UMETA(DisplayName = "Material"),     // 재료 (라운드 종료 시 유지)
    Recipe     UMETA(DisplayName = "Recipe"),       // 레시피
    Weapon     UMETA(DisplayName = "Weapon"),       // 무기 (칼, 단검)
    Tool       UMETA(DisplayName = "Tool"),         // 도구
    Consumable UMETA(DisplayName = "Consumable"),   // 소비 (포션, 음식)
    Structure  UMETA(DisplayName = "Structure")     // 방호벽, 함정
};

UCLASS(BlueprintType)
class TREASUREHUNT_API UItemData : public UDataAsset
{
    GENERATED_BODY()

public:
    // 아이템 고유 ID
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
    FName ItemID;

    // 인게임 표시 이름
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display")
    FText DisplayName;

    // 아이템 설명
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display", meta = (MultiLine = "true"))
    FText Description;

    // 아이콘 (UI 표시용)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display")
    TObjectPtr<UTexture2D> Icon;

    // 차지하는 인벤토리 칸 수 (기획서: 1~4칸)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Inventory",
        meta = (ClampMin = "1", ClampMax = "10"))
    int32 SlotsRequired = 1;

    // 그리드에서 차지하는 모양 (기획서 v1.2: 5x4 그리드)
        // 예: 포션 1x1, 단검 2x1, 칼 3x1, 방호벽 3x2
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Inventory",
        meta = (ClampMin = "1", ClampMax = "5"))
    int32 GridWidth = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Inventory",
        meta = (ClampMin = "1", ClampMax = "4"))
    int32 GridHeight = 1;

    // 한 슬롯에 최대 몇 개 쌓을 수 있는지 (스택 가능 여부)
    // 1이면 스택 불가 (무기 등), 99 같으면 스택 가능 (포션, 재료)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Inventory",
        meta = (ClampMin = "1"))
    int32 MaxStackSize = 1;

    // 아이템 종류
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Type")
    EItemType ItemType = EItemType::Material;

    // 라운드 종료 시 유지되는가?
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Type")
    bool bSurvivesRoundEnd = true;

    // 도적이 강탈 가능한가?
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Type")
    bool bCanBeStolen = true;
};