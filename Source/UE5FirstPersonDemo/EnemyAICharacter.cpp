// EnemyAICharacter.cpp - 敌人AI实现

#include "EnemyAICharacter.h"
#include "FirstPersonDemoCharacter.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Perception/PawnSensingComponent.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyAI, Warning, All);

AEnemyAICharacter::AEnemyAICharacter()
{
	// 启用复制
	bReplicates = true;
	bReplicateMovement = true;

	// AI感知组件
	UPawnSensingComponent* PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->SetPeripheralVisionAngle(90.0f);
	PawnSensing->SightRadius = 2000.0f;
	PawnSensing->HearingThreshold = 1000.0f;
	PawnSensing->LOSHearingThreshold = 1500.0f;

	// 初始化属性
	MaxHealth = 100.0f;
	Health = MaxHealth;
	PatrolSpeed = 150.0f;
	ChaseSpeed = 400.0f;
	AttackRange = 150.0f;
	AttackDamage = 15.0f;
	SightRange = 2000.0f;
	SightAngle = 90.0f;
	AttackCooldown = 1.5f;
	ScoreReward = 100;

	CurrentState = EEnemyState::Idle;
	CurrentTarget = nullptr;
	CurrentPatrolIndex = 0;
	LastAttackTime = 0.0f;
	bIsDead = false;

	// 设置网络更新频率
	NetUpdateFrequency = 50.0f;
	MinNetUpdateFrequency = 10.0f;

	// 初始化角色移动
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f);

	// 设置AI控制器
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyAICharacter::BeginPlay()
{
	Super::BeginPlay();

	// 初始状态为巡逻
	SetEnemyState(EEnemyState::Patrol);
}

void AEnemyAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	// 根据状态执行行为
	switch (CurrentState)
	{
	case EEnemyState::Patrol:
		// 巡逻逻辑由行为树处理
		break;

	case EEnemyState::Chase:
		if (CurrentTarget && !CurrentTarget->IsHidden())
		{
			// 移动向目标
			FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			AddMovementInput(Direction);

			// 检测是否进入攻击范围
			if (IsPlayerInAttackRange())
			{
				SetEnemyState(EEnemyState::Attack);
			}
		}
		else
		{
			// 目标丢失，返回巡逻
			SetEnemyState(EEnemyState::Patrol);
		}
		break;

	case EEnemyState::Attack:
		if (CurrentTarget && IsPlayerInAttackRange())
		{
			// 面向目标
			FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(
				GetActorLocation(), CurrentTarget->GetActorLocation());
			SetActorRotation(FRotator(0.0f, TargetRotation.Yaw, 0.0f));

			// 执行攻击
			float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurrentTime - LastAttackTime >= AttackCooldown)
			{
				PerformMeleeAttack();
			}
		}
		else
		{
			// 目标超出范围，追逐
			SetEnemyState(EEnemyState::Chase);
		}
		break;

	default:
		break;
	}
}

void AEnemyAICharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyAICharacter, Health);
	DOREPLIFETIME(AEnemyAICharacter, CurrentState);
	DOREPLIFETIME(AEnemyAICharacter, CurrentTarget);
	DOREPLIFETIME(AEnemyAICharacter, bIsDead);
}

float AEnemyAICharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.0f;
	}

	if (HasAuthority())
	{
		const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);

		// 播放受伤音效
		if (HurtSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, HurtSound, GetActorLocation());
		}

		// 检测攻击者并设置为目标
		if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(DamageCauser))
		{
			if (CurrentState == EEnemyState::Idle || CurrentState == EEnemyState::Patrol)
			{
				CurrentTarget = Player;
				SetEnemyState(EEnemyState::Chase);
			}
		}

		// 检测死亡
		if (Health <= 0.0f)
		{
			Die();

			// 奖励击杀者
			if (AFirstPersonDemoCharacter* Killer = Cast<AFirstPersonDemoCharacter>(DamageCauser))
			{
				Killer->AddScore(ScoreReward);
				Killer->KillCount++;
			}
		}

		return ActualDamage;
	}

	return DamageAmount;
}

void AEnemyAICharacter::PerformMeleeAttack()
{
	if (bIsDead || !CurrentTarget)
	{
		return;
	}

	LastAttackTime = GetWorld()->GetTimeSeconds();

	// 播放攻击动画
	PlayAttackAnimation();

	// 播放攻击音效
	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}

	// 检测攻击范围内的玩家
	TArray<AActor*> OverlappingActors;
	GetCapsuleComponent()->GetOverlappingActors(OverlappingActors, AFirstPersonDemoCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(Actor))
		{
			// 计算距离
			float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
			if (Distance <= AttackRange)
			{
				// 造成伤害
				UGameplayStatics::ApplyDamage(Player, AttackDamage, GetController(), this, UDamageType::StaticClass());
			}
		}
	}
}

bool AEnemyAICharacter::IsPlayerInAttackRange() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
	return Distance <= AttackRange;
}

bool AEnemyAICharacter::IsPlayerInSight() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	// 检测距离
	float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (Distance > SightRange)
	{
		return false;
	}

	// 检测角度
	FVector DirectionToTarget = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector ForwardVector = GetActorForwardVector();
	float DotProduct = FVector::DotProduct(ForwardVector, DirectionToTarget);
	float Angle = FMath::Acos(DotProduct) * (180.0f / PI);

	return Angle <= SightAngle;
}

void AEnemyAICharacter::SetEnemyState(EEnemyState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;

	// 根据状态调整速度
	switch (NewState)
	{
	case EEnemyState::Patrol:
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		break;

	case EEnemyState::Chase:
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
		break;

	case EEnemyState::Attack:
		GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		break;

	case EEnemyState::Dead:
		GetCharacterMovement()->DisableMovement();
		break;

	default:
		break;
	}
}

EEnemyState AEnemyAICharacter::GetEnemyState() const
{
	return CurrentState;
}

void AEnemyAICharacter::ChaseTarget(AActor* Target)
{
	CurrentTarget = Target;
	SetEnemyState(EEnemyState::Chase);
}

void AEnemyAICharacter::AttackTarget(AActor* Target)
{
	CurrentTarget = Target;
	SetEnemyState(EEnemyState::Attack);
}

void AEnemyAICharacter::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	SetEnemyState(EEnemyState::Dead);

	// 播放死亡动画
	PlayDeathAnimation();

	// 播放死亡音效
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// 禁用碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 启用布娃娃物理
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 设置销毁定时器
	if (HasAuthority())
	{
		FTimerHandle DeathTimer;
		GetWorld()->GetTimerManager().SetTimer(DeathTimer, [this]()
		{
			Destroy();
		}, 5.0f, false);
	}
}

float AEnemyAICharacter::GetHealthPercent() const
{
	return (MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f;
}

void AEnemyAICharacter::PlayAttackAnimation()
{
	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}
}

void AEnemyAICharacter::PlayDeathAnimation()
{
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}

AFirstPersonDemoCharacter* AEnemyAICharacter::FindNearestPlayer() const
{
	AFirstPersonDemoCharacter* NearestPlayer = nullptr;
	float NearestDistance = SightRange;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(It->Get()))
		{
			if (!Player->bIsDead)
			{
				float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
				if (Distance < NearestDistance && IsPlayerInSight())
				{
					NearestDistance = Distance;
					NearestPlayer = Player;
				}
			}
		}
	}

	return NearestPlayer;
}

void AEnemyAICharacter::OnRep_Health()
{
	// 生命值变化时的视觉效果（如果有）
}

void AEnemyAICharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		GetMesh()->SetSimulatePhysics(true);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
