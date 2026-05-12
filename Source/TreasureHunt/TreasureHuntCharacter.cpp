// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreasureHuntCharacter.h"
#include "TreasureHuntProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TreasureHuntGameState.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ATreasureHuntCharacter

ATreasureHuntCharacter::ATreasureHuntCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

}

//////////////////////////////////////////////////////////////////////////// Input

void ATreasureHuntCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ATreasureHuntCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATreasureHuntCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATreasureHuntCharacter::Look);

		// Attacking
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ATreasureHuntCharacter::OnAttackInput);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ATreasureHuntCharacter::OnSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ATreasureHuntCharacter::OnSprintStop);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
	
}


void ATreasureHuntCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ATreasureHuntCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}



void ATreasureHuntCharacter::OnAttackInput()
{
	UE_LOG(LogTemp, Warning, TEXT("[Client] Attack input received"));
	Server_TryAttack();
}

void ATreasureHuntCharacter::Server_TryAttack_Implementation()
{
	if (!HasAuthority()) return;

	if (ATreasureHuntGameState* GS = Cast<ATreasureHuntGameState>(GetWorld()->GetGameState()))
	{
		if (GS->CurrentPhase == EGamePhase::Day)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Cannot attack during day phase"));
			return;
		}
	}

	// 카메라 위치/방향 기준으로 트레이스
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * AttackRange);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);  // 자기 자신은 안 맞게

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Pawn, Params);

	// 디버그용: 트레이스 라인 시각화 (1초간)
	DrawDebugLine(GetWorld(), Start, End,
		bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.0f);

	if (bHit)
	{
		ATreasureHuntCharacter* HitCharacter = Cast<ATreasureHuntCharacter>(Hit.GetActor());
		if (HitCharacter && HitCharacter != this)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Server] %s attacked %s for %.1f damage"),
				*GetName(), *HitCharacter->GetName(), AttackDamage);

			HitCharacter->Health = FMath::Max(0.0f, HitCharacter->Health - AttackDamage);

			UE_LOG(LogTemp, Warning, TEXT("[Server] %s Health = %.1f"),
				*HitCharacter->GetName(), HitCharacter->Health);

			// 사망 체크
			if (HitCharacter->Health <= 0.0f && !HitCharacter->bIsDead)
			{
				HitCharacter->bIsDead = true;
				HitCharacter->Multicast_OnDeath();
			}
		}
	}
}




// ===== 멀티플레이 테스트용 체력 시스템 구현 =====

void ATreasureHuntCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Health 변수를 모든 클라이언트에 복제하라고 등록
	DOREPLIFETIME(ATreasureHuntCharacter, Health);
	DOREPLIFETIME(ATreasureHuntCharacter, bIsDead);
	DOREPLIFETIME(ATreasureHuntCharacter, Stamina);
	DOREPLIFETIME(ATreasureHuntCharacter, bIsSprinting);
}

void ATreasureHuntCharacter::OnTestDamageInput()
{
	// 클라이언트에서 호출됨 → 서버에 요청만 보냄
	UE_LOG(LogTemp, Warning, TEXT("[Client] F key pressed - requesting damage to server"));
	Server_TakeTestDamage(10.0f);
}

void ATreasureHuntCharacter::Server_TakeTestDamage_Implementation(float Amount)
{
	// 이 함수는 무조건 서버에서만 실행됨 (Server RPC라서)
	// 그래도 안전을 위해 권한 체크 한 번 더
	if (!HasAuthority()) return;

	Health = FMath::Max(0.0f, Health - Amount);

	UE_LOG(LogTemp, Warning, TEXT("[Server] %s took %.1f damage. Health = %.1f"),
		*GetName(), Amount, Health);

	if (Health <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		Multicast_OnDeath();
	}
	// Health 변수가 바뀌면 자동으로 모든 클라에게 복제되고
	// 각 클라에서 OnRep_Health()가 호출됨
}

void ATreasureHuntCharacter::OnRep_Health()
{
	// 각 클라이언트에서 Health 변경을 감지했을 때 호출됨
	// 나중에 여기서 UI 갱신, 피격 이펙트 등 처리
	UE_LOG(LogTemp, Warning, TEXT("[Client OnRep] %s Health changed to %.1f"),
		*GetName(), Health);
}


// 사망시 효과
void ATreasureHuntCharacter::Multicast_OnDeath_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("[All] %s died"), *GetName());

    // 입력 잠금 (자기 컨트롤러만)
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
    }

    // 콜리전 끄기 (다른 플레이어가 통과 가능, 시체 상태)
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 이동 정지
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }
}

// ===== 기력(Stamina) + 달리기 시스템 =====

void ATreasureHuntCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 서버에서만 기력 계산 (결과는 Replicated로 클라에 자동 전달)
	if (!HasAuthority()) return;

	if (bIsSprinting)
	{
		// 달리는 중 → 기력 감소
		Stamina = FMath::Max(0.0f, Stamina - StaminaDrainRate * DeltaSeconds);

		// 기력이 0이 되면 강제로 달리기 중단
		if (Stamina <= 0.0f)
		{
			Server_StopSprint();
		}
	}
	else
	{
		// 걷거나 멈춤 → 기력 회복
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRecoveryRate * DeltaSeconds);
	}
}

void ATreasureHuntCharacter::OnSprintStart()
{
	// 클라이언트에서 Shift 눌렸을 때 서버에 요청
	Server_StartSprint();
}

void ATreasureHuntCharacter::OnSprintStop()
{
	// 클라이언트에서 Shift 뗐을 때 서버에 요청
	Server_StopSprint();
}

void ATreasureHuntCharacter::Server_StartSprint_Implementation()
{
	if (!HasAuthority()) return;

	// 기력이 0이면 달리기 시작 못 함
	if (Stamina <= 0.0f) return;

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	UE_LOG(LogTemp, Warning, TEXT("[Server] %s 달리기 시작. Stamina: %.1f"), *GetName(), Stamina);
}

void ATreasureHuntCharacter::Server_StopSprint_Implementation()
{
	if (!HasAuthority()) return;

	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	UE_LOG(LogTemp, Warning, TEXT("[Server] %s 달리기 중단. Stamina: %.1f"), *GetName(), Stamina);
}

void ATreasureHuntCharacter::OnRep_Stamina()
{
	// Stamina가 바뀔 때 클라이언트에서 자동 호출
	// 나중에 여기서 기력 UI 바 업데이트 할 거야
	UE_LOG(LogTemp, Warning, TEXT("[Client OnRep] %s Stamina: %.1f"), *GetName(), Stamina);
}