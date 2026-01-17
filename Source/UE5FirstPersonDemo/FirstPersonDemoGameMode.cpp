// FirstPersonDemoGameMode.cpp - 游戏模式实现

#include "FirstPersonDemoGameMode.h"
#include "FirstPersonDemoCharacter.h"
#include "EnemyAICharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameMode, Warning, All);

AFirstPersonDemoGameMode::AFirstPersonDemoGameMode()
{
	// 启用网络复制
	bReplicates = true;

	// 设置默认玩家控制器类
	PlayerControllerClass = APlayerController::StaticClass();

	// 设置默认角色类
	DefaultPawnClass = AFirstPersonDemoCharacter::StaticClass();

	// 初始化游戏设置
	VictoryCondition = EVictoryCondition::Score;
	TargetScore = 1000;
	TargetKillCount = 10;
	GameTimeLimit = 600.0f; // 10分钟
	RemainingTime = GameTimeLimit;

	CurrentWave = 0;
	MaxWaves = 5;
	EnemiesPerWave = 5;
	EnemySpawnInterval = 2.0f;
	WaveInterval = 10.0f;
	RespawnDelay = 5.0f;

	CurrentGameState = EGameState::Waiting;

	// 默认敌人生成位置（将在BeginPlay中初始化）
	EnemySpawnLocations.Add(FVector(500.0f, 500.0f, 100.0f));
	EnemySpawnLocations.Add(FVector(-500.0f, 500.0f, 100.0f));
	EnemySpawnLocations.Add(FVector(500.0f, -500.0f, 100.0f));
	EnemySpawnLocations.Add(FVector(-500.0f, -500.0f, 100.0f));
	EnemySpawnLocations.Add(FVector(0.0f, 1000.0f, 100.0f));

	// 默认玩家出生位置
	PlayerStartLocations.Add(FVector(0.0f, 0.0f, 100.0f));
	PlayerStartLocations.Add(FVector(200.0f, 0.0f, 100.0f));
	PlayerStartLocations.Add(FVector(-200.0f, 0.0f, 100.0f));
	PlayerStartLocations.Add(FVector(0.0f, 200.0f, 100.0f));
}

void AFirstPersonDemoGameMode::BeginPlay()
{
	Super::BeginPlay();

	InitializeGame();
}

void AFirstPersonDemoGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentGameState == EGameState::InProgress)
	{
		// 更新游戏计时器
		if (VictoryCondition == EVictoryCondition::TimeLimit)
		{
			RemainingTime -= DeltaTime;
			if (RemainingTime <= 0.0f)
			{
				RemainingTime = 0.0f;

				// 时间到，检查最高分玩家获胜
				if (AFirstPersonDemoCharacter* Winner = GetHighestScoringPlayer())
				{
					EndGame(Winner);
				}
			}
		}

		// 检查胜利条件
		CheckVictoryCondition();
	}
}

void AFirstPersonDemoGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogGameMode, Log, TEXT("Player joined: %s"), *NewPlayer->GetName());

	// 如果游戏正在进行，生成玩家
	if (CurrentGameState == EGameState::InProgress)
	{
		if (APawn* Pawn = NewPlayer->GetPawn())
		{
			if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(Pawn))
			{
				FVector SpawnLocation = ChoosePlayerStart(Player);
				Player->SetActorLocation(SpawnLocation);
			}
		}
	}

	// 如果等待中的玩家足够多，开始游戏
	if (GetNumPlayers() >= 1)
	{
		StartGame();
	}
}

void AFirstPersonDemoGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	UE_LOG(LogGameMode, Log, TEXT("Player left: %s"), *Exiting->GetName());

	// 从待重生列表中移除
	if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(Exiting->GetPawn()))
	{
		PlayersToRespawn.Remove(Player);
	}

	// 检查是否所有玩家都已离开
	if (GetNumPlayers() == 0)
	{
		EndGame(nullptr);
	}
}

void AFirstPersonDemoGameMode::InitializeGame()
{
	CurrentGameState = EGameState::Waiting;
	RemainingTime = GameTimeLimit;
	CurrentWave = 0;

	// 清理现有敌人生成位置
	EnemySpawnLocations.Empty();

	// 从场景中获取所有敌人生成点
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("EnemySpawn"), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		EnemySpawnLocations.Add(Actor->GetActorLocation());
	}

	// 如果没有找到生成点，使用默认位置
	if (EnemySpawnLocations.Num() == 0)
	{
		EnemySpawnLocations.Add(FVector(500.0f, 500.0f, 100.0f));
		EnemySpawnLocations.Add(FVector(-500.0f, 500.0f, 100.0f));
		EnemySpawnLocations.Add(FVector(500.0f, -500.0f, 100.0f));
		EnemySpawnLocations.Add(FVector(-500.0f, -500.0f, 100.0f));
		EnemySpawnLocations.Add(FVector(0.0f, 1000.0f, 100.0f));
	}

	// 清理现有玩家出生点
	PlayerStartLocations.Empty();

	// 从场景中获取所有玩家出生点
	FoundActors.Empty();
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("PlayerStart"), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		PlayerStartLocations.Add(Actor->GetActorLocation());
	}

	// 如果没有找到出生点，使用默认位置
	if (PlayerStartLocations.Num() == 0)
	{
		PlayerStartLocations.Add(FVector(0.0f, 0.0f, 100.0f));
		PlayerStartLocations.Add(FVector(200.0f, 0.0f, 100.0f));
		PlayerStartLocations.Add(FVector(-200.0f, 0.0f, 100.0f));
		PlayerStartLocations.Add(FVector(0.0f, 200.0f, 100.0f));
	}

	OnGameStateChangedDelegate.Broadcast(CurrentGameState);
}

void AFirstPersonDemoGameMode::StartGame()
{
	if (CurrentGameState != EGameState::Waiting)
	{
		return;
	}

	CurrentGameState = EGameState::InProgress;
	OnGameStateChangedDelegate.Broadcast(CurrentGameState);

	UE_LOG(LogGameMode, Log, TEXT("Game started!"));

	// 启动游戏计时器
	if (VictoryCondition == EVictoryCondition::TimeLimit)
	{
		GetWorld()->GetTimerManager().SetTimer(GameTimerHandle, [this]()
		{
			if (AFirstPersonDemoCharacter* Winner = GetHighestScoringPlayer())
			{
				EndGame(Winner);
			}
		}, GameTimeLimit, false);
	}

	// 生成第一波敌人
	SpawnEnemyWave(1);
}

void AFirstPersonDemoGameMode::EndGame(AFirstPersonDemoCharacter* Winner)
{
	if (CurrentGameState == EGameState::GameOver)
	{
		return;
	}

	CurrentGameState = EGameState::GameOver;
	OnGameStateChangedDelegate.Broadcast(CurrentGameState);

	// 清理所有计时器
	GetWorld()->GetTimerManager().ClearTimer(GameTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(EnemySpawnTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(WaveTimerHandle);

	if (Winner)
	{
		UE_LOG(LogGameMode, Log, TEXT("Game Over! Winner: %s with score: %d"),
			*Winner->GetName(), Winner->Score);
		OnGameWinnerDelegate.Broadcast(Winner);
	}
	else
	{
		UE_LOG(LogGameMode, Log, TEXT("Game Over! No winner."));
		OnGameWinnerDelegate.Broadcast(nullptr);
	}

	// 广播游戏结束消息
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (Winner)
			{
				FString Message = FString::Printf(TEXT("Game Over! Winner: %s with score: %d"),
					*Winner->GetName(), Winner->Score);
				PC->ClientMessage(Message);
			}
			else
			{
				PC->ClientMessage(TEXT("Game Over!"));
			}
		}
	}
}

void AFirstPersonDemoGameMode::OnPlayerDeath(AFirstPersonDemoCharacter* DeadPlayer)
{
	if (!DeadPlayer)
	{
		return;
	}

	UE_LOG(LogGameMode, Log, TEXT("Player died: %s"), *DeadPlayer->GetName());

	// 添加到待重生列表
	PlayersToRespawn.Add(DeadPlayer);

	// 设置重生定时器
	FTimerHandle RespawnTimer;
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, [this, DeadPlayer]()
	{
		if (DeadPlayer && !DeadPlayer->IsPendingKillPending())
		{
			DeadPlayer->Respawn();
			PlayersToRespawn.Remove(DeadPlayer);
		}
	}, RespawnDelay, false);

	// 检查胜利条件
	CheckVictoryCondition();
}

void AFirstPersonDemoGameMode::OnEnemyDeath(AEnemyAICharacter* DeadEnemy)
{
	if (!DeadEnemy)
	{
		return;
	}

	// 从已生成列表中移除
	SpawnedEnemies.Remove(DeadEnemy);

	// 检查当前波次是否完成
	if (SpawnedEnemies.Num() == 0 && CurrentWave < MaxWaves)
	{
		// 生成下一波
		GetWorld()->GetTimerManager().SetTimer(WaveTimerHandle, [this]()
		{
			SpawnNextWave();
		}, WaveInterval, false);
	}
}

void AFirstPersonDemoGameMode::SpawnEnemy()
{
	if (!EnemyClass || EnemySpawnLocations.Num() == 0)
	{
		return;
	}

	// 随机选择生成位置
	int32 RandomIndex = FMath::RandRange(0, EnemySpawnLocations.Num() - 1);
	FVector SpawnLocation = EnemySpawnLocations[RandomIndex];

	// 生成敌人
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AEnemyAICharacter* Enemy = GetWorld()->SpawnActor<AEnemyAICharacter>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
	{
		SpawnedEnemies.Add(Enemy);
		UE_LOG(LogGameMode, Log, TEXT("Spawned enemy at: %s"), *SpawnLocation.ToString());
	}
}

void AFirstPersonDemoGameMode::SpawnEnemyWave(int32 WaveNumber)
{
	if (WaveNumber > MaxWaves)
	{
		return;
	}

	CurrentWave = WaveNumber;
	int32 EnemiesToSpawn = EnemiesPerWave + (WaveNumber - 1) * 2; // 每波增加2个敌人

	UE_LOG(LogGameMode, Log, TEXT("Spawning wave %d with %d enemies"), WaveNumber, EnemiesToSpawn);

	// 生成敌人
	for (int32 i = 0; i < EnemiesToSpawn; i++)
	{
		GetWorld()->GetTimerManager().SetTimer(EnemySpawnTimerHandle, [this]()
		{
			SpawnEnemy();
		}, i * EnemySpawnInterval, false);
	}
}

void AFirstPersonDemoGameMode::SpawnNextWave()
{
	if (CurrentWave < MaxWaves)
	{
		SpawnEnemyWave(CurrentWave + 1);
	}
	else
	{
		// 所有波次完成，检查胜利
		if (AFirstPersonDemoCharacter* Winner = GetHighestScoringPlayer())
		{
			EndGame(Winner);
		}
	}
}

AFirstPersonDemoCharacter* AFirstPersonDemoGameMode::GetHighestScoringPlayer() const
{
	AFirstPersonDemoCharacter* HighestScorer = nullptr;
	int32 HighestScore = -1;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(It->Get()))
		{
			if (Player->Score > HighestScore)
			{
				HighestScore = Player->Score;
				HighestScorer = Player;
			}
		}
	}

	return HighestScorer;
}

void AFirstPersonDemoGameMode::CheckVictoryCondition()
{
	if (CurrentGameState != EGameState::InProgress)
	{
		return;
	}

	switch (VictoryCondition)
	{
	case EVictoryCondition::Score:
		{
			// 检查是否有玩家达到目标分数
			for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
			{
				if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(It->Get()))
				{
					if (Player->Score >= TargetScore)
					{
						EndGame(Player);
						return;
					}
				}
			}
		}
		break;

	case EVictoryCondition::KillCount:
		{
			// 检查是否有玩家达到目标击杀数
			for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
			{
				if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(It->Get()))
				{
					if (Player->KillCount >= TargetKillCount)
					{
						EndGame(Player);
						return;
					}
				}
			}
		}
		break;

	case EVictoryCondition::Survival:
		{
			// 检查所有波次是否完成
			if (CurrentWave >= MaxWaves && SpawnedEnemies.Num() == 0)
			{
				if (AFirstPersonDemoCharacter* Winner = GetHighestScoringPlayer())
				{
					EndGame(Winner);
				}
			}
		}
		break;

	default:
		break;
	}
}

void AFirstPersonDemoGameMode::UpdatePlayerScores()
{
	// 更新所有玩家的得分信息
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(It->Get()))
		{
			OnPlayerScoreChangedDelegate.Broadcast(Player, Player->Score);
		}
	}
}

FVector AFirstPersonDemoGameMode::ChoosePlayerStart(AFirstPersonDemoCharacter* Player)
{
	if (PlayerStartLocations.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	// 简单轮询选择出生点
	static int32 StartIndex = 0;
	FVector SpawnLocation = PlayerStartLocations[StartIndex % PlayerStartLocations.Num()];
	StartIndex++;

	return SpawnLocation;
}

void AFirstPersonDemoGameMode::OnRep_GameState()
{
	OnGameStateChangedDelegate.Broadcast(CurrentGameState);
}
