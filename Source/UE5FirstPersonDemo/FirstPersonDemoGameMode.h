// FirstPersonDemoGameMode.h - 游戏模式，管理游戏规则、得分和胜利条件

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FirstPersonDemoGameMode.generated.h"

class AFirstPersonDemoCharacter;
class AEnemyAICharacter;

/**
 * 游戏胜利条件
 */
UENUM(BlueprintType)
enum class EVictoryCondition : uint8
{
	Score			UMETA(DisplayName = "达到目标分数"),
	KillCount		UMETA(DisplayName = "达到目标击杀数"),
	TimeLimit		UMETA(DisplayName = "时间限制"),
	Survival		UMETA(DisplayName = "生存模式"),
	TeamDeathmatch	UMETA(DisplayName = "团队死斗")
};

/**
 * 游戏状态
 */
UENUM(BlueprintType)
enum class EGameState : uint8
{
	Waiting		UMETA(DisplayName = "等待中"),
	InProgress	UMETA(DisplayName = "进行中"),
	GameOver	UMETA(DisplayName = "游戏结束")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateChanged, EGameState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerScoreChanged, AFirstPersonDemoCharacter*, Player, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameWinner, AFirstPersonDemoCharacter*, Winner);

/**
 * 游戏模式类 - 管理游戏逻辑、得分系统和胜利条件
 */
UCLASS()
class AFirstPersonDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFirstPersonDemoGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** 初始化游戏 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void InitializeGame();

	/** 开始游戏 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void StartGame();

	/** 结束游戏 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void EndGame(AFirstPersonDemoCharacter* Winner);

	/** 玩家死亡处理 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void OnPlayerDeath(AFirstPersonDemoCharacter* DeadPlayer);

	/** 敌人死亡处理 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void OnEnemyDeath(AEnemyAICharacter* DeadEnemy);

	/** 生成敌人 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void SpawnEnemy();

	/** 生成多组敌人波次 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void SpawnEnemyWave(int32 WaveNumber);

	/** 获取当前游戏状态 */
	UFUNCTION(BlueprintPure, Category = Game)
	EGameState GetGameState() const { return CurrentGameState; }

	/** 获取剩余时间 */
	UFUNCTION(BlueprintPure, Category = Game)
	float GetRemainingTime() const { return RemainingTime; }

	/** 获取当前波次 */
	UFUNCTION(BlueprintPure, Category = Game)
	int32 GetCurrentWave() const { return CurrentWave; }

	/** 获取最高分玩家 */
	UFUNCTION(BlueprintPure, Category = Game)
	AFirstPersonDemoCharacter* GetHighestScoringPlayer() const;

	/** 检查胜利条件 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void CheckVictoryCondition();

	/** 广播游戏状态变化 */
	UPROPERTY(BlueprintAssignable, Category = Game)
	FOnGameStateChanged OnGameStateChangedDelegate;

	/** 广播玩家得分变化 */
	UPROPERTY(BlueprintAssignable, Category = Game)
	FOnPlayerScoreChanged OnPlayerScoreChangedDelegate;

	/** 广播游戏胜利者 */
	UPROPERTY(BlueprintAssignable, Category = Game)
	FOnGameWinner OnGameWinnerDelegate;

protected:
	/** 游戏状态 */
	UPROPERTY(ReplicatedUsing=OnRep_GameState, VisibleAnywhere, BlueprintReadOnly, Category = Game)
	EGameState CurrentGameState;

	/** 胜利条件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	EVictoryCondition VictoryCondition;

	/** 目标分数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	int32 TargetScore;

	/** 目标击杀数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	int32 TargetKillCount;

	/** 游戏时间限制 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	float GameTimeLimit;

	/** 剩余时间 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Game)
	float RemainingTime;

	/** 当前波次 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Game)
	int32 CurrentWave;

	/** 最大波次 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	int32 MaxWaves;

	/** 每波敌人数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	int32 EnemiesPerWave;

	/** 敌人生成间隔 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	float EnemySpawnInterval;

	/** 敌人生成位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	TArray<FVector> EnemySpawnLocations;

	/** 敌人类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	TSubclassOf<AEnemyAICharacter> EnemyClass;

	/** 玩家开始位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	TArray<FVector> PlayerStartLocations;

	/** 重生延迟 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	float RespawnDelay;

	/** 波次间隔时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
	float WaveInterval;

private:
	/** 网络：游戏状态复制回调 */
	UFUNCTION()
	void OnRep_GameState();

	/** 更新玩家得分 */
	void UpdatePlayerScores();

	/** 生成下一波敌人 */
	void SpawnNextWave();

	/** 选择玩家出生点 */
	FVector ChoosePlayerStart(AFirstPersonDemoCharacter* Player);

	/** 计时器句柄 */
	FTimerHandle GameTimerHandle;
	FTimerHandle EnemySpawnTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle WaveTimerHandle;

	/** 待重生的玩家 */
	TArray<TWeakObjectPtr<AFirstPersonDemoCharacter>> PlayersToRespawn;

	/** 已生成的敌人 */
	TArray<TWeakObjectPtr<AEnemyAICharacter>> SpawnedEnemies;
};
