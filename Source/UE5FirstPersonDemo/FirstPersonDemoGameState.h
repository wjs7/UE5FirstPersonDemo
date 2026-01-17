// FirstPersonDemoGameState.h - 游戏状态，用于网络复制游戏数据

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FirstPersonDemoGameState.generated.h"

UENUM(BlueprintType)
enum class EMatchState : uint8
{
	Waiting		UMETA(DisplayName = "等待中"),
	Starting	UMETA(DisplayName = "准备开始"),
	InProgress	UMETA(DisplayName = "进行中"),
	Ending		UMETA(DisplayName = "即将结束"),
	Finished	UMETA(DisplayName = "已结束")
};

/**
 * 玩家分数信息结构体
 */
USTRUCT(BlueprintType)
struct FPlayerScoreData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Score;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 KillCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DeathCount;

	FPlayerScoreData()
		: PlayerName(TEXT(""))
		, Score(0)
		, KillCount(0)
		, DeathCount(0)
	{
	}
};

/**
 * 游戏状态类 - 存储和复制游戏相关数据
 */
UCLASS()
class AFirstPersonDemoGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFirstPersonDemoGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 获取当前匹配状态 */
	UFUNCTION(BlueprintPure, Category = Game)
	EMatchState GetMatchState() const { return CurrentMatchState; }

	/** 获取当前波次 */
	UFUNCTION(BlueprintPure, Category = Game)
	int32 GetCurrentWave() const { return CurrentWave; }

	/** 获取剩余时间 */
	UFUNCTION(BlueprintPure, Category = Game)
	float GetRemainingTime() const { return RemainingTime; }

	/** 获取玩家分数列表 */
	UFUNCTION(BlueprintPure, Category = Game)
	TArray<FPlayerScoreData> GetPlayerScores() const { return PlayerScores; }

	/** 获取领先玩家 */
	UFUNCTION(BlueprintPure, Category = Game)
	FPlayerScoreData GetLeadingPlayer() const;

	/** 更新玩家分数 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void UpdatePlayerScore(const FString& PlayerName, int32 Score, int32 Kills, int32 Deaths);

	/** 设置匹配状态 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void SetMatchState(EMatchState NewState);

	/** 增加当前波次 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void IncrementWave() { CurrentWave++; }

protected:
	/** 当前匹配状态 */
	UPROPERTY(ReplicatedUsing=OnRep_MatchState, VisibleAnywhere, BlueprintReadOnly, Category = Game)
	EMatchState CurrentMatchState;

	/** 当前波次 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Game)
	int32 CurrentWave;

	/** 剩余时间 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Game)
	float RemainingTime;

	/** 玩家分数列表 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Game)
	TArray<FPlayerScoreData> PlayerScores;

	/** 排序后的玩家分数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Game)
	TArray<FPlayerScoreData> SortedPlayerScores;

private:
	/** 网络：匹配状态复制回调 */
	UFUNCTION()
	void OnRep_MatchState();

	/** 排序玩家分数 */
	void SortPlayerScores();
};
