# UE5 First Person Multiplayer Demo

基于 Unreal Engine 5 第一人称模板的多人网络对战游戏demo，包含完整的敌人AI、战斗系统、得分系统和游戏胜利机制。

## 功能特性

### 1. 玩家系统
- 第一人称视角控制
- 武器射击系统
- 生命值和受伤系统
- 死亡和重生机制
- 多人网络对战支持

### 2. 敌人AI系统
- 智能巡逻行为
- 玩家检测和追逐
- 近战攻击能力
- 多种AI状态（闲置、巡逻、追逐、攻击、死亡）
- 行为树驱动的AI决策

### 3. 战斗系统
- 射线检测射击
- 近战攻击
- 伤害计算和应用
- 击中反馈特效
- 音效系统

### 4. 得分和胜利机制
- 多种胜利条件：
  - 达到目标分数
  - 达到目标击杀数
  - 时间限制模式
  - 生存波次模式
- 实时得分显示
- 排行榜系统
- 游戏结束检测

### 5. 多人网络对战
- 服务器-客户端架构
- 网络复制和同步
- RPC调用支持
- 玩家状态同步
- 匹配和会话管理

### 6. 波次系统
- 多波次敌人进攻
- 每波难度递增
- 波次间隔时间
- 自适应敌人生成

## 项目结构

```
UE5FirstPersonDemo/
├── Source/UE5FirstPersonDemo/
│   ├── UE5FirstPersonDemo.h/cpp          # 主模块
│   ├── FirstPersonDemoCharacter.h/cpp    # 玩家角色类
│   ├── EnemyAICharacter.h/cpp            # 敌人AI角色类
│   ├── EnemyAIController.h/cpp           # 敌人AI控制器
│   ├── FirstPersonDemoGameMode.h/cpp     # 游戏模式
│   └── FirstPersonDemoGameState.h/cpp    # 游戏状态
├── Config/                               # 配置文件
├── Content/                              # 游戏资产（蓝图、材质等）
└── README.md                             # 项目说明
```

## 技术实现

### 核心类说明

#### AFirstPersonDemoCharacter
玩家角色类，继承自ACharacter，支持：
- 网络复制 (`bReplicates = true`)
- 生命值系统 (`Health`, `MaxHealth`)
- 射击系统 (`FireWeapon`, `ServerFireWeapon`)
- 伤害处理 (`TakeDamage`)
- 死亡和重生 (`Die`, `Respawn`)

#### AEnemyAICharacter
敌人AI角色类，支持：
- AI状态机 (EEnemyState枚举)
- 行为树集成
- 巡逻点系统
- 视野检测
- 近战攻击

#### AEnemyAIController
敌人AI控制器，负责：
- 行为树执行
- 黑板管理
- 目标选择
- AI决策

#### AFirstPersonDemoGameMode
游戏模式类，管理：
- 游戏规则
- 胜利条件检测
- 敌人生成
- 玩家重生
- 波次管理

### 网络架构

项目使用UE5内置的网络系统：
- **服务器权威**：所有游戏逻辑在服务器执行
- **属性复制**：使用`Replicated`和`ReplicatedUsing`标记
- **RPC调用**：
  - `Server` RPC：客户端调用服务器执行
  - `NetMulticast` RPC：服务器广播所有客户端
  - `Client` RPC：服务器调用特定客户端

### AI实现

敌人AI使用行为树系统：
1. **感知系统**：UPawnSensingComponent检测玩家
2. **黑板**：存储AI状态和目标信息
3. **行为树**：定义AI行为逻辑
4. **AI控制器**：执行行为树并更新黑板

## 如何使用

### 环境要求
- Unreal Engine 5.3+
- Visual Studio 2019+ (Windows)
- C++17支持

### 编译步骤
1. 克隆项目到本地：
   ```bash
   git clone https://github.com/wjs7/UE5FirstPersonDemo.git
   ```
2. 使用UE5打开`.uproject`文件
3. 在UE5编辑器中编译C++代码

### 运行游戏
1. 启动服务器：
   ```
   YourGame.exe -log -server
   ```
2. 启动客户端：
   ```
   YourGame.exe 127.0.0.1 -log
   ```

### 编辑器中测试多人游戏
1. 点击 Play 按钮旁的下拉箭头
2. 选择 "Number of Players" 为 2 或更多
3. 点击 Play 启动多人游戏

## 游戏玩法

### 基本操作
- **WASD** - 移动
- **鼠标** - 视角控制
- **空格** - 跳跃
- **鼠标左键** - 射击

### 游戏目标
根据选择的胜利条件：
- **分数模式**：击杀敌人获得分数，先达到目标分数的玩家获胜
- **击杀模式**：先达到目标击杀数的玩家获胜
- **生存模式**：在所有敌人波次中生存下来，分数最高的玩家获胜

## 扩展功能

项目预留了以下扩展接口：
- 更多武器类型
- 道具和掉落系统
- 团队模式
- 排行榜和成就系统
- 更多AI行为类型

## 依赖插件

项目使用以下UE5内置插件：
- EnhancedInput - 增强输入系统
- GameplayAbilities - 游戏能力系统
- AIModule - AI模块
- OnlineSubsystem - 在线子系统

## 贡献指南

欢迎提交问题和改进建议！

## 许可证

本项目仅供学习交流使用。

## 作者

Created with UE5 First Person Template
