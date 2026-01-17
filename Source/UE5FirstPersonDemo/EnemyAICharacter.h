// EnemyAICharacter.h - 敌人AI角色类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyAICharacter.generated.h"

class UBehaviorTree;
class AFirstPersonDemoCharacter;
class UAnimMontage;
class USoundBase;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Patrol		UMETA(DisplayName = "Patrol"),
	Chase		UMETA(DisplayName = "Chase"),
	Attack		UMETA(DisplayName = "Attack"),
	Dead		UMETA(DisplayName = "Dead")
};

/**
 * 敌人AI角色 - 支持巡逻、追逐和攻击玩家
 */
UCLASS()
class AEnemyAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyAICharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** 获取生命值百分比 */
	UFUNCTION(BlueprintPure, Category = Gameplay)
	float GetHealthPercent() const;

	/** 造成近战伤害 */
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void PerformMeleeAttack();

	/** 检测玩家是否在攻击范围内 */
	UFUNCTION(BlueprintPure, Category = AI)
	bool IsPlayerInAttackRange() const;

	/** 检测玩家是否在视野内 */
	UFUNCTION(BlueprintPure, Category = AI)
	bool IsPlayerInSight() const;

	/** 设置敌人状态 */
	UFUNCTION(BlueprintCallable, Category = AI)
	void SetEnemyState(EEnemyState NewState);

	/** 获取敌人状态 */
	UFUNCTION(BlueprintPure, Category = AI)
	EEnemyState GetEnemyState() const { return CurrentState; }

	/** 追逐目标 */
	UFUNCTION(BlueprintCallable, Category = AI)
	void ChaseTarget(AActor* Target);

	/** 攻击目标 */
	UFUNCTION(BlueprintCallable, Category = AI)
	void AttackTarget(AActor* Target);

	/** 死亡处理 */
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void Die();

	/** 网络复制 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** 行为树 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AI)
	UBehaviorTree* BehaviorTree;

	/** 生命值 */
	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category = Gameplay)
	float Health;

	/** 最大生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float MaxHealth;

	/** 移动速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float PatrolSpeed;

	/** 追逐速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float ChaseSpeed;

	/** 攻击范围 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	float AttackRange;

	/** 攻击伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	float AttackDamage;

	/** 视野范围 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	float SightRange;

	/** 视野角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	float SightAngle;

	/** 攻击间隔 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	float AttackCooldown;

	/** 攻击动画 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UAnimMontage* AttackMontage;

	/** 死亡动画 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	UAnimMontage* DeathMontage;

	/** 攻击音效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* AttackSound;

	/** 死亡音效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* DeathSound;

	/** 受伤音效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* HurtSound;

	/** 当前状态 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = AI)
	EEnemyState CurrentState;

	/** 当前目标 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = AI)
	AActor* CurrentTarget;

	/** 巡逻点数组 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	TArray<AActor*> PatrolPoints;

	/** 当前巡逻点索引 */
	int32 CurrentPatrolIndex;

	/** 上次攻击时间 */
	float LastAttackTime;

	/** 是否死亡 */
	UPROPERTY(ReplicatedUsing=OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category = Gameplay)
	bool bIsDead;

	/** 死亡后的分数奖励 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	int32 ScoreReward;

private:
	/** 网络：生命值复制回调 */
	UFUNCTION()
	void OnRep_Health();

	/** 网络：死亡状态复制回调 */
	UFUNCTION()
	void OnRep_IsDead();

	/** 播放攻击动画 */
	void PlayAttackAnimation();

	/** 播放死亡动画 */
	void PlayDeathAnimation();

	/** 寻找最近的可攻击玩家 */
	AFirstPersonDemoCharacter* FindNearestPlayer() const;
};
