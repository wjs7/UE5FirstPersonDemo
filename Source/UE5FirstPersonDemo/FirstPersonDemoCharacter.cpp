// FirstPersonDemoCharacter.cpp - 第一人称角色实现

#include "FirstPersonDemoCharacter.h"
#include "FirstPersonDemoGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AFirstPersonDemoCharacter

AFirstPersonDemoCharacter::AFirstPersonDemoCharacter()
{
	// 设置角色大小
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 启用复制
	bReplicates = true;
	bReplicateMovement = true;
	SetReplicatingMovement(true);

	// 创建第一人称相机
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // 眼睛高度
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// 创建第一人称手臂网格（本地玩家看到）
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetupAttachment(FirstPersonCameraComponent);
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->CastShadow = false;
	FirstPersonMesh->SetRelativeLocation(FVector(30.f, 0.f, -30.f));

	// 创建第三人称角色网格（其他玩家看到）
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonMesh"));
	ThirdPersonMesh->SetOwnerNoSee(true);
	ThirdPersonMesh->SetupAttachment(GetCapsuleComponent());
	ThirdPersonMesh->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
	ThirdPersonMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	// 初始化属性
	MaxHealth = 100.0f;
	Health = MaxHealth;
	WeaponDamage = 20.0f;
	Score = 0;
	KillCount = 0;
	bIsDead = false;

	FireRate = 0.15f; // 每秒约6.7发
	bIsFiring = false;
	LastFireTime = 0.0f;

	// 移动设置
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// 网络更新频率
	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 20.0f;
}

void AFirstPersonDemoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFirstPersonDemoCharacter, Health);
	DOREPLIFETIME(AFirstPersonDemoCharacter, Score);
	DOREPLIFETIME(AFirstPersonDemoCharacter, KillCount);
	DOREPLIFETIME(AFirstPersonDemoCharacter, bIsDead);
	DOREPLIFETIME_CONDITION(AFirstPersonDemoCharacter, bIsFiring, COND_SkipOwner);
}

void AFirstPersonDemoCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeHealth();
}

void AFirstPersonDemoCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 持续射击逻辑
	if (bIsFiring && !bIsDead)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastFireTime >= FireRate)
		{
			FireWeapon();
		}
	}
}

void AFirstPersonDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 设置游戏玩法输入绑定
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 跳跃
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AFirstPersonDemoCharacter::OnStartJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFirstPersonDemoCharacter::OnStopJump);

		// 移动
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFirstPersonDemoCharacter::Move);

		// 视角
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFirstPersonDemoCharacter::Look);

		// 射击
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AFirstPersonDemoCharacter::OnStartFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AFirstPersonDemoCharacter::OnStopFire);
	}
	else
	{
		UE_LOG(LogFPChar, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetName());
	}
}

void AFirstPersonDemoCharacter::OnStartJump()
{
	if (!bIsDead)
	{
		Jump();
	}
}

void AFirstPersonDemoCharacter::OnStopJump()
{
	StopJumping();
}

void AFirstPersonDemoCharacter::OnStartFire()
{
	if (!bIsDead)
	{
		bIsFiring = true;
		FireWeapon();
	}
}

void AFirstPersonDemoCharacter::OnStopFire()
{
	bIsFiring = false;
}

void AFirstPersonDemoCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller || bIsDead)
	{
		return;
	}

	const FVector2D MoveVector = Value.Get<FVector2D>();

	// 前后移动
	if (MoveVector.Y != 0.f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, MoveVector.Y);
	}

	// 左右移动
	if (MoveVector.X != 0.f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, MoveVector.X);
	}
}

void AFirstPersonDemoCharacter::Look(const FInputActionValue& Value)
{
	if (!Controller || bIsDead)
	{
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (LookAxisVector.X != 0.f)
	{
		AddControllerYawInput(LookAxisVector.X);
	}

	if (LookAxisVector.Y != 0.f)
	{
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AFirstPersonDemoCharacter::FireWeapon()
{
	if (bIsDead)
	{
		return;
	}

	LastFireTime = GetWorld()->GetTimeSeconds();

	// 播放射击音效
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// 播放枪口火焰
	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, FirstPersonMesh, FName("Muzzle"));
	}

	// 射线检测
	FVector HitLocation;
	AActor* HitActor;
	if (WeaponTrace(HitLocation, HitActor))
	{
		// 造成伤害
		if (HasAuthority())
		{
			ApplyPointDamage(HitActor, WeaponDamage, HitLocation);
		}
		else
		{
			ServerFireWeapon(FirstPersonCameraComponent->GetComponentLocation(),
				FirstPersonCameraComponent->GetForwardVector());
		}
	}
}

void AFirstPersonDemoCharacter::ServerFireWeapon_Implementation(FVector_NetQuantize10 Origin, FVector_NetQuantize10 Direction)
{
	if (bIsDead)
	{
		return;
	}

	// 播放射击音效
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// 播放枪口火焰
	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, FirstPersonMesh, FName("Muzzle"));
	}

	// 射线检测
	FVector Start = Origin;
	FVector End = Origin + (Direction * 10000.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, Params))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			ApplyPointDamage(HitActor, WeaponDamage, HitResult.Location);
		}
	}
}

bool AFirstPersonDemoCharacter::ServerFireWeapon_Validate(FVector_NetQuantize10 Origin, FVector_NetQuantize10 Direction)
{
	return true;
}

bool AFirstPersonDemoCharacter::WeaponTrace(FVector& OutHitLocation, AActor*& OutHitActor)
{
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 10000.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, Params);

	if (bHit)
	{
		OutHitLocation = HitResult.Location;
		OutHitActor = HitResult.GetActor();
	}
	else
	{
		OutHitLocation = End;
		OutHitActor = nullptr;
	}

	// 调试绘制射线
	// DrawDebugLine(GetWorld(), Start, bHit ? HitResult.Location : End, FColor::Red, false, 1.0f);

	return bHit;
}

void AFirstPersonDemoCharacter::ApplyPointDamage(AActor* HitActor, float Damage, const FVector& HitLocation)
{
	if (!HitActor || HitActor == this)
	{
		return;
	}

	UGameplayStatics::ApplyPointDamage(HitActor, Damage, GetActorForwardVector(),
		FHitResult(), GetController(), this, UDamageType::StaticClass());

	// 播放击中特效
	if (UParticleSystem* HitEffect = Cast<UParticleSystem>(StaticLoadObject(UParticleSystem::StaticClass(),
		nullptr, TEXT("/Game/FirstPerson/Particles/P_HitEffect"))))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, HitLocation);
	}
}

float AFirstPersonDemoCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.0f;
	}

	// 服务器处理伤害
	if (HasAuthority())
	{
		const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);

		OnHealthChangedDelegate.Broadcast(Health);

		if (Health <= 0.0f)
		{
			OnDeath();

			// 通知攻击者
			if (AFirstPersonDemoCharacter* Attacker = Cast<AFirstPersonDemoCharacter>(DamageCauser))
			{
				Attacker->AddScore(100);
				Attacker->KillCount++;
			}
		}

		return ActualDamage;
	}

	return DamageAmount;
}

void AFirstPersonDemoCharacter::OnDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	// 禁用移动和碰撞
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 播放死亡动画
	if (UAnimMontage* DeathAnim = Cast<UAnimMontage>(StaticLoadObject(UAnimMontage::StaticClass(),
		nullptr, TEXT("/Game/FirstPerson/Animations/DeathAnim"))))
	{
		PlayAnimMontage(DeathAnim);
	}

	// 多播死亡
	MulticastOnDeath();

	// 通知游戏模式
	if (AFirstPersonDemoGameMode* GM = Cast<AFirstPersonDemoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->OnPlayerDeath(this);
	}

	// 触发委托
	OnDeathDelegate.Broadcast();
}

void AFirstPersonDemoCharacter::MulticastOnDeath_Implementation()
{
	// 本地玩家死亡效果
	FirstPersonMesh->SetHiddenInGame(true);
	ThirdPersonMesh->SetHiddenInGame(false);
	ThirdPersonMesh->SetSimulatePhysics(true);

	if (AController* PC = GetController())
	{
		PC->UnPossess();
	}
}

void AFirstPersonDemoCharacter::OnRep_Health()
{
	OnHealthChangedDelegate.Broadcast(Health);
}

void AFirstPersonDemoCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		FirstPersonMesh->SetHiddenInGame(true);
		ThirdPersonMesh->SetHiddenInGame(false);
		ThirdPersonMesh->SetSimulatePhysics(true);
	}
}

float AFirstPersonDemoCharacter::GetHealthPercent() const
{
	return (MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f;
}

void AFirstPersonDemoCharacter::DealDamage(AActor* DamagedActor, float Damage)
{
	if (HasAuthority())
	{
		UGameplayStatics::ApplyDamage(DamagedActor, Damage, GetController(), this, UDamageType::StaticClass());
	}
}

void AFirstPersonDemoCharacter::AddScore(int32 Points)
{
	if (HasAuthority())
	{
		Score += Points;
	}
}

void AFirstPersonDemoCharacter::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsDead = false;
	Health = MaxHealth;
	OnHealthChangedDelegate.Broadcast(Health);

	// 重置角色状态
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->MovementMode = MOVE_Falling;
	FirstPersonMesh->SetHiddenInGame(false);
	ThirdPersonMesh->SetHiddenInGame(true);
	ThirdPersonMesh->SetSimulatePhysics(false);

	// 重生
	if (AFirstPersonDemoGameMode* GM = Cast<AFirstPersonDemoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		FVector SpawnLocation = GM->ChoosePlayerStart(this);
		SetActorLocation(SpawnLocation);
	}

	if (AController* PC = GetController())
	{
		PC->Possess(this);
	}
}

void AFirstPersonDemoCharacter::InitializeHealth()
{
	Health = MaxHealth;
	OnHealthChangedDelegate.Broadcast(Health);
}
