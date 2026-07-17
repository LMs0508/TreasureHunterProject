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
#include "InventoryComponent.h"

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

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
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
	// UI 열려있을 때는 우클릭 중에만 카메라 회전 허용
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->bShowMouseCursor && !PC->IsInputKeyDown(EKeys::RightMouseButton))
		{
			return;
		}
	}

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}



void ATreasureHuntCharacter::OnAttackInput()
{
	// UI�� ���������� ���� ����
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] bShowMouseCursor: %d"), PC->bShowMouseCursor);
		if (PC->bShowMouseCursor) return; // ���콺 Ŀ���� ���̸� UI ���� ����
	}

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

	// ī�޶� ��ġ/���� �������� Ʈ���̽�
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * AttackRange);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);  // �ڱ� �ڽ��� �� �°�

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Pawn, Params);

	// ����׿�: Ʈ���̽� ���� �ð�ȭ (1�ʰ�)
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

			// ��� üũ
			if (HitCharacter->Health <= 0.0f && !HitCharacter->bIsDead)
			{
				HitCharacter->bIsDead = true;
				HitCharacter->Multicast_OnDeath();
			}
		}
	}
}




// ===== ��Ƽ�÷��� �׽�Ʈ�� ü�� �ý��� ���� =====

void ATreasureHuntCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Health ������ ��� Ŭ���̾�Ʈ�� �����϶�� ���
	DOREPLIFETIME(ATreasureHuntCharacter, Health);
	DOREPLIFETIME(ATreasureHuntCharacter, bIsDead);
	DOREPLIFETIME(ATreasureHuntCharacter, Stamina);
	DOREPLIFETIME(ATreasureHuntCharacter, bIsSprinting);
}

void ATreasureHuntCharacter::OnTestDamageInput()
{
	// Ŭ���̾�Ʈ���� ȣ��� �� ������ ��û�� ����
	UE_LOG(LogTemp, Warning, TEXT("[Client] F key pressed - requesting damage to server"));
	Server_TakeTestDamage(10.0f);
}

void ATreasureHuntCharacter::Server_TakeTestDamage_Implementation(float Amount)
{
	// �� �Լ��� ������ ���������� ����� (Server RPC��)
	// �׷��� ������ ���� ���� üũ �� �� ��
	if (!HasAuthority()) return;

	Health = FMath::Max(0.0f, Health - Amount);

	UE_LOG(LogTemp, Warning, TEXT("[Server] %s took %.1f damage. Health = %.1f"),
		*GetName(), Amount, Health);

	if (Health <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		Multicast_OnDeath();
	}
	// Health ������ �ٲ�� �ڵ����� ��� Ŭ�󿡰� �����ǰ�
	// �� Ŭ�󿡼� OnRep_Health()�� ȣ���
}

void ATreasureHuntCharacter::OnRep_Health()
{
	// �� Ŭ���̾�Ʈ���� Health ������ �������� �� ȣ���
	// ���߿� ���⼭ UI ����, �ǰ� ����Ʈ �� ó��
	UE_LOG(LogTemp, Warning, TEXT("[Client OnRep] %s Health changed to %.1f"),
		*GetName(), Health);
}


// ����� ȿ��
void ATreasureHuntCharacter::Multicast_OnDeath_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("[All] %s died"), *GetName());

    // �Է� ��� (�ڱ� ��Ʈ�ѷ���)
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
    }

    // �ݸ��� ���� (�ٸ� �÷��̾ ��� ����, ��ü ����)
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // �̵� ����
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }
}

// ===== ���(Stamina) + �޸��� �ý��� =====

void ATreasureHuntCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// ���������� ��� ��� (����� Replicated�� Ŭ�� �ڵ� ����)
	if (!HasAuthority()) return;

	if (bIsSprinting)
	{
		// �޸��� �� �� ��� ����
		Stamina = FMath::Max(0.0f, Stamina - StaminaDrainRate * DeltaSeconds);

		// ����� 0�� �Ǹ� ������ �޸��� �ߴ�
		if (Stamina <= 0.0f)
		{
			Server_StopSprint();
		}
	}
	else
	{
		// �Ȱų� ���� �� ��� ȸ��
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRecoveryRate * DeltaSeconds);
	}
}

void ATreasureHuntCharacter::OnSprintStart()
{
	// Ŭ���̾�Ʈ���� Shift ������ �� ������ ��û
	Server_StartSprint();
}

void ATreasureHuntCharacter::OnSprintStop()
{
	// Ŭ���̾�Ʈ���� Shift ���� �� ������ ��û
	Server_StopSprint();
}

void ATreasureHuntCharacter::Server_StartSprint_Implementation()
{
	if (!HasAuthority()) return;

	// ����� 0�̸� �޸��� ���� �� ��
	if (Stamina <= 0.0f) return;

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	UE_LOG(LogTemp, Warning, TEXT("[Server] %s �޸��� ����. Stamina: %.1f"), *GetName(), Stamina);
}

void ATreasureHuntCharacter::Server_StopSprint_Implementation()
{
	if (!HasAuthority()) return;

	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	UE_LOG(LogTemp, Warning, TEXT("[Server] %s �޸��� �ߴ�. Stamina: %.1f"), *GetName(), Stamina);
}

void ATreasureHuntCharacter::OnRep_Stamina()
{
	if (IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client OnRep] %s Stamina: %.1f"), *GetName(), Stamina);
	}
}