// EnemyAIController.cpp - 敌人AI控制器实现

#include "EnemyAIController.h"
#include "EnemyAICharacter.h"
#include "FirstPersonDemoCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyController, Warning, All);

AEnemyAIController::AEnemyAIController()
{
	// 创建组件
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));

	// 设置感知组件
	bSetControlRotationFromPawnOrientation = false;
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	// 初始化黑板键名
	TargetKeyName = FName("Target");
	CurrentStateKeyName = FName("CurrentState");
	AttackRangeKeyName = FName("AttackRange");
	SightRangeKeyName = FName("SightRange");
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!EnemyCharacter || EnemyCharacter->bIsDead)
	{
		return;
	}

	// 更新黑板
	UpdateBlackboard();

	// 如果没有目标，尝试查找玩家
	if (!GetTarget())
	{
		if (AActor* Player = FindPlayer())
		{
			SetTarget(Player);
		}
	}
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	EnemyCharacter = Cast<AEnemyAICharacter>(InPawn);

	if (EnemyCharacter)
	{
		// 初始化并启动行为树
		if (EnemyCharacter->BehaviorTree)
		{
			BehaviorTreeAsset = EnemyCharacter->BehaviorTree;

			// 初始化黑板
			UBlackboardData* BlackboardAsset = BehaviorTreeAsset->BlackboardAsset;
			if (UseBlackboard(BlackboardAsset, BlackboardComponent))
			{
				InitializeBlackboard();
				RunBehaviorTree(BehaviorTreeAsset);
			}
		}
	}
}

void AEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();

	// 清理行为树
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree();
	}

	EnemyCharacter = nullptr;
}

void AEnemyAIController::SetTarget(AActor* NewTarget)
{
	if (BlackboardComponent)
	{
		BlackboardComponent->SetValueAsObject(TargetKeyName, NewTarget);

		if (EnemyCharacter && NewTarget)
		{
			EnemyCharacter->ChaseTarget(NewTarget);
		}
	}
}

AActor* AEnemyAIController::GetTarget() const
{
	if (BlackboardComponent)
	{
		return Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetKeyName));
	}
	return nullptr;
}

UBlackboardComponent* AEnemyAIController::GetBlackboardComponent() const
{
	return BlackboardComponent;
}

void AEnemyAIController::InitializeBlackboard()
{
	if (!BlackboardComponent || !EnemyCharacter)
	{
		return;
	}

	// 初始化黑板值
	BlackboardComponent->SetValueAsObject(TargetKeyName, nullptr);
	BlackboardComponent->SetValueAsEnum(CurrentStateKeyName, static_cast<uint8>(EEnemyState::Idle));
	BlackboardComponent->SetValueAsFloat(AttackRangeKeyName, EnemyCharacter->AttackRange);
	BlackboardComponent->SetValueAsFloat(SightRangeKeyName, EnemyCharacter->SightRange);
}

void AEnemyAIController::UpdateBlackboard()
{
	if (!BlackboardComponent || !EnemyCharacter)
	{
		return;
	}

	// 更新状态
	BlackboardComponent->SetValueAsEnum(CurrentStateKeyName, static_cast<uint8>(EnemyCharacter->GetEnemyState()));

	// 检查目标是否有效
	AActor* CurrentTarget = GetTarget();
	if (CurrentTarget)
	{
		if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(CurrentTarget))
		{
			if (Player->bIsDead || Player->Health <= 0.0f)
			{
				SetTarget(nullptr);
			}
		}
	}
}

AActor* AEnemyAIController::FindPlayer() const
{
	if (!EnemyCharacter)
	{
		return nullptr;
	}

	AFirstPersonDemoCharacter* NearestPlayer = nullptr;
	float NearestDistance = EnemyCharacter->SightRange;

	// 查找所有玩家
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		if (AFirstPersonDemoCharacter* Player = Cast<AFirstPersonDemoCharacter>(It->Get()))
		{
			if (!Player->bIsDead && Player->Health > 0.0f)
			{
				float Distance = FVector::Dist(EnemyCharacter->GetActorLocation(), Player->GetActorLocation());

				if (Distance < NearestDistance)
				{
					// 检查视野
					FVector DirectionToPlayer = (Player->GetActorLocation() - EnemyCharacter->GetActorLocation()).GetSafeNormal();
					FVector ForwardVector = EnemyCharacter->GetActorForwardVector();
					float DotProduct = FVector::DotProduct(ForwardVector, DirectionToPlayer);
					float Angle = FMath::Acos(DotProduct) * (180.0f / PI);

					if (Angle <= EnemyCharacter->SightAngle)
					{
						// 检查视线遮挡
						FHitResult HitResult;
						FCollisionQueryParams Params;
						Params.AddIgnoredActor(EnemyCharacter);

						if (GetWorld()->LineTraceSingleByChannel(HitResult,
							EnemyCharacter->GetActorLocation(),
							Player->GetActorLocation(),
							ECC_Pawn,
							Params))
						{
							if (HitResult.GetActor() == Player)
							{
								NearestDistance = Distance;
								NearestPlayer = Player;
							}
						}
					}
				}
			}
		}
	}

	return NearestPlayer;
}
