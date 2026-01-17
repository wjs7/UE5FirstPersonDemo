// EnemyAIController.h - 敌人AI控制器

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AEnemyAICharacter;
class UBlackboardComponent;
class UBehaviorTreeComponent;

/**
 * 敌人AI控制器 - 使用行为树控制敌人行为
 */
UCLASS()
class AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** 设置新目标 */
	UFUNCTION(BlueprintCallable, Category = AI)
	void SetTarget(AActor* NewTarget);

	/** 获取当前目标 */
	UFUNCTION(BlueprintPure, Category = AI)
	AActor* GetTarget() const;

	/** 获取黑板组件 */
	UFUNCTION(BlueprintPure, Category = AI)
	UBlackboardComponent* GetBlackboardComponent() const;

protected:
	/** 行为树组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI)
	UBehaviorTreeComponent* BehaviorTreeComponent;

	/** 黑板组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI)
	UBlackboardComponent* BlackboardComponent;

	/** 行为树资源 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	UBehaviorTree* BehaviorTreeAsset;

	/** 黑板键名 */
	FName TargetKeyName;
	FName CurrentStateKeyName;
	FName AttackRangeKeyName;
	FName SightRangeKeyName;

private:
	/** 初始化黑板 */
	void InitializeBlackboard();

	/** 更新黑板 */
	void UpdateBlackboard();

	/** 查找玩家 */
	AActor* FindPlayer() const;

	/** 敌人角色引用 */
	UPROPERTY()
	AEnemyAICharacter* EnemyCharacter;
};
