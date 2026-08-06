#include "StatueActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AStatueActor::AStatueActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
}

void AStatueActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // bActivated만 복제. SecretNumber는 여기 없음 (의도적)
    DOREPLIFETIME(AStatueActor, bActivated);
}

void AStatueActor::InitStatue(int32 InNumber, int32 InIndex)
{
    // 서버 전용
    if (!HasAuthority()) { return; }

    SecretNumber = InNumber;
    PasswordIndex = InIndex;
}

void AStatueActor::ClearNumber()
{
    if (!HasAuthority()) { return; }

    SecretNumber = -1;
    PasswordIndex = -1;
}

void AStatueActor::SetActivated(bool bNewActivated)
{
    if (!HasAuthority()) { return; }

    bActivated = bNewActivated;
    OnRep_Activated();   // 서버 본인도 반영 (리슨서버 호스트용)
}

void AStatueActor::OnRep_Activated()
{
    // 활성화 시각 처리 (예: 머티리얼 변경, 빛남 등)
    // 지금은 비워두고 M6-B에서 채움
}