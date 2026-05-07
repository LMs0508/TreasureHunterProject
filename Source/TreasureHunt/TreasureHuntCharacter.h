// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TreasureHuntCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ATreasureHuntCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;


	
public:
	ATreasureHuntCharacter();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }


public:
	// 공격 입력 (클라이언트에서 실행, 서버에 요청 전송)
	void OnAttackInput();

	// 서버에서 트레이스 + 데미지 적용 (Server RPC)
	UFUNCTION(Server, Reliable)
	void Server_TryAttack();

	// 공격 데미지 (직업별 보너스 붙일 자리)
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 10.0f;

	// 공격 사거리 (근접 무기 기준)
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 200.0f;



	// ===== 멀티플레이 테스트용 체력 시스템 =====
public:
	// 체력 (서버 → 모든 클라이언트로 자동 복제)
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float Health = 100.0f;

	// 사망 상태 (서버 → 클라 자동 복제)
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Stats")
	bool bIsDead = false;

	// 사망 시 모든 클라이언트에게 알리는 멀티캐스트 RPC
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();




	// F키 입력 시 호출 (클라이언트에서 실행)
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void OnTestDamageInput();

	// 서버에 데미지 처리 요청 (Server RPC)
	UFUNCTION(Server, Reliable)
	void Server_TakeTestDamage(float Amount);

	// Health가 변경되면 모든 클라이언트에서 자동 호출됨
	UFUNCTION()
	void OnRep_Health();

	// Replication 시스템에 어떤 변수를 복제할지 알려주는 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};

