// FirstPersonDemoGameState.cpp - 游戏状态实现

#include "FirstPersonDemoGameState.h"
#include "Net/UnrealNetwork.h"
#include "Algo/Sort.h"

AFirstPersonDemoGameState::AFirstPersonDemoGameState()
{
	// 启用复制
	bReplicates = true;

	// 初始化状态
	CurrentMatchState = EMatchState::Waiting;
	CurrentWave = 0;
	RemainingTime = 0.0f;
}

void AFirstPersonDemoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFirstPersonDemoGameState, CurrentMatchState);
	DOREPLIFETIME(AFirstPersonDemoGameState, CurrentWave);
	DOREPLIFETIME(AFirstPersonDemoGameState, RemainingTime);
	DOREPLIFETIME(AFirstPersonDemoGameState, PlayerScores);
}

FPlayerScoreData AFirstPersonDemoGameState::GetLeadingPlayer() const
{
	if (SortedPlayerScores.Num() > 0)
	{
		return SortedPlayerScores[0];
	}

	return FPlayerScoreData();
}

void AFirstPersonDemoGameState::UpdatePlayerScore(const FString& PlayerName, int32 Score, int32 Kills, int32 Deaths)
{
	// 查找现有玩家分数
	for (FPlayerScoreData& ScoreData : PlayerScores)
	{
		if (ScoreData.PlayerName == PlayerName)
		{
			ScoreData.Score = Score;
			ScoreData.KillCount = Kills;
			ScoreData.DeathCount = Deaths;
			SortPlayerScores();
			return;
		}
	}

	// 添加新玩家分数
	FPlayerScoreData NewScoreData;
	NewScoreData.PlayerName = PlayerName;
	NewScoreData.Score = Score;
	NewScoreData.KillCount = Kills;
	NewScoreData.DeathCount = Deaths;

	PlayerScores.Add(NewScoreData);
	SortPlayerScores();
}

void AFirstPersonDemoGameState::SetMatchState(EMatchState NewState)
{
	if (CurrentMatchState != NewState)
	{
		CurrentMatchState = NewState;
		OnRep_MatchState();
	}
}

void AFirstPersonDemoGameState::OnRep_MatchState()
{
	// 匹配状态变化时的处理
}

void AFirstPersonDemoGameState::SortPlayerScores()
{
	// 按分数降序排序
	SortedPlayerScores = PlayerScores;
	SortedPlayerScores.Sort([](const FPlayerScoreData& A, const FPlayerScoreData& B)
	{
		return A.Score > B.Score;
	});
}
