// FirstPersonDemoCharacter.h - 主角角色类，支持多人网络对战

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FirstPersonDemoCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);

/**
 * 第一人称角色类 - 支持多人对战、攻击、受伤和死亡
 */
UCLASS(config=Game)
class AFirstPersonDemoCharacter : public ACharacter
{
	GENERATED_BODY()

	/** 映射上下文 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** 跳跃输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** 移动输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** 视角输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** 射击输入动作 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;

public:
	AFirstPersonDemoCharacter();

	/** 网络复制 - 属性 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 赋予pawn输入控制权 */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** 开始播放时 */
	virtual void BeginPlay() override;

	/** 每帧更新 */
	virtual void Tick(float DeltaTime) override;

protected:
	/** 触发跳跃 */
	void OnStartJump();
	void OnStopJump();

	/** 触发射击 */
	void OnStartFire();
	void OnStopFire();

	/** 触发移动 */
	void Move(const FInputActionValue& Value);

	/** 触发视角旋转 */
	void Look(const FInputActionValue& Value);

	/** 重置跳跃动作 */
	void ResetJump();

	/** 射击处理 */
	void FireWeapon();

	/** 服务器射击 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireWeapon(FVector_NetQuantize10 Origin, FVector_NetQuantize10 Direction);

	/** 处理伤害 */
	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** 死亡处理 */
	UFUNCTION()
	void OnDeath();

	/** 网络：死亡处理 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnDeath();

public:
	/** 第一人称相机 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* FirstPersonCameraComponent;

	/** 第一人称手臂网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
	USkeletalMeshComponent* FirstPersonMesh;

	/** 角色网格（用于其他玩家看到的角色） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
	USkeletalMeshComponent* ThirdPersonMesh;

	/** 生命值 */
	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category = Gameplay)
	float Health;

	/** 最大生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float MaxHealth;

	/** 武器伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float WeaponDamage;

	/** 得分 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Gameplay)
	int32 Score;

	/** 杀敌数 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Gameplay)
	int32 KillCount;

	/** 是否死亡 */
	UPROPERTY(ReplicatedUsing=OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category = Gameplay)
	bool bIsDead;

	/** 死亡事件 */
	UPROPERTY(BlueprintAssignable, Category = Gameplay)
	FOnDeathEvent OnDeathDelegate;

	/** 生命值改变事件 */
	UPROPERTY(BlueprintAssignable, Category = Gameplay)
	FOnHealthChanged OnHealthChangedDelegate;

	/** 攻击间隔 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float FireRate;

	/** 射击音效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* FireSound;

	/** 击中特效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UParticleSystem* MuzzleFlash;

	/** 获取当前生命值百分比 */
	UFUNCTION(BlueprintPure, Category = Gameplay)
	float GetHealthPercent() const;

	/** 造成伤害 */
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void DealDamage(AActor* DamagedActor, float Damage);

	/** 增加得分 */
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void AddScore(int32 Points);

	/** 重生 */
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	void Respawn();

	/** 初始化生命值 */
	UFUNCTION()
	void InitializeHealth();

protected:
	/** 计时器句柄 */
	FTimerHandle FireTimerHandle;

	/** 是否正在射击 */
	bool bIsFiring;

	/** 上次射击时间 */
	float LastFireTime;

	/** 网络：生命值复制回调 */
	UFUNCTION()
	void OnRep_Health();

	/** 网络：死亡状态复制回调 */
	UFUNCTION()
	void OnRep_IsDead();

	/** 射线检测 - 服务器使用 */
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	bool WeaponTrace(FVector& OutHitLocation, AActor*& OutHitActor);

	/** 造成点伤害 */
	void ApplyPointDamage(AActor* HitActor, float Damage, const FVector& HitLocation);
};
