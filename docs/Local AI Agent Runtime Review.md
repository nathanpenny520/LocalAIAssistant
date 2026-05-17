# Local AI Agent Runtime Review

## 系统整体定位

当前系统本质上已经不是普通聊天程序，而是一个：

```text
LLM 驱动的本地 Agent Runtime
```

核心目标：

```text
AI → 规划 → 执行 → 获取结果 → 再规划
```

当前架构已经具备：

- Task Planning
- Command Execution
- Safety Validation
- Undo System
- Multi-step Loop
- Cross-platform Shell Runtime

属于一个较完整的 Agent 执行框架雏形。

------

# 一、系统架构分析

当前执行链路：

```text
LLM
 ↓
TaskEngine
 ↓
OperationPlan
 ↓
SafetyChecker
 ↓
CommandExecutor
 ↓
AgentLoop
 ↓
反馈给LLM
```

各模块职责：

| 模块              | 职责                          |
| ----------------- | ----------------------------- |
| `TaskEngine`      | 解析 AI 返回的 TASK_PLAN      |
| `OperationPlan`   | Agent 的结构化 IR（中间表示） |
| `CommandExecutor` | Shell/File 执行器             |
| `SafetyChecker`   | 安全策略引擎                  |
| `OperationUndo`   | 可撤销执行层                  |
| `AgentLoop`       | Agent Runtime 主循环          |

------

# 二、当前存在的核心问题

------

# 1. Agent Loop 未真正闭环（最严重）

## 问题现象

当前系统：

```text
执行一次命令后停止
```

表现：

- AI 无法继续下一步
- 只执行第一次 TASK_PLAN
- Agent 卡住
- 无法持续自治

------

## 原因分析

当前代码：

```cpp
emit executionResultReady(feedback, m_sessionId);
```

这里只发送了 signal：

```text
executionResultReady
```

但：

```text
没有任何地方保证：
LLM收到反馈 → 再调用 continueWithResponse()
```

因此：

当前真正流程：

```text
AI
 ↓
生成 TASK_PLAN
 ↓
executeAndContinue()
 ↓
emit executionResultReady()
 ↓
流程终止
```

而不是：

```text
AI
 ↓
生成 TASK_PLAN
 ↓
执行
 ↓
结果反馈
 ↓
LLM继续推理
 ↓
生成新 TASK_PLAN
 ↓
继续执行
```

------

## 根本问题

缺少：

```text
Agent Runtime Orchestrator
```

即：

```text
executionResultReady
    ↓
LLM Request
    ↓
LLM Response
    ↓
continueWithResponse()
```

整个自动闭环不存在。

------

## 改进建议

## 推荐架构

```text
AgentLoop
   ↓
LLMWorker (async)
   ↓
HTTP API / Local Model
   ↓
LLM Response
   ↓
continueWithResponse()
```

------

## 推荐实现

新增：

```cpp
class AgentOrchestrator
```

负责：

- 监听 executionResultReady
- 调用模型 API
- 获取回复
- 自动调用 continueWithResponse()

------

## 建议改为异步

当前：

```text
同步阻塞式
```

建议：

```text
异步事件驱动
```

使用：

```cpp
QNetworkAccessManager
signals/slots
```

------

# 2. 命令输出严重截断

------

## 问题现象

AI：

```text
无法获取完整命令结果
```

导致：

- 无法分析错误
- 无法继续推理
- npm/pip/cmake 类任务失败
- 编译错误丢失

------

## 原因分析

代码：

```cpp
if (output.length() > 80)
    output = output.left(77) + "...";
```

stdout 被强制截断为：

```text
80字符
```

这对于 agent 是灾难性的。

------

## 导致的问题

例如：

```bash
npm install
```

真实错误：

```text
Missing dependency:
node-gyp failed:
Python not found
```

AI 实际看到：

```text
npm ERR...
```

因此 AI：

- 无法修复
- 会无限重试
- 会生成错误计划

------

## 改进建议

------

## 不要截断 stdout

至少：

```cpp
const int MAX_OUTPUT = 8000;
```

------

## 更好的方案

使用：

```text
完整日志文件
```

例如：

```text
logs/iteration_3.log
```

AI 只看到：

```json
{
  "summary": "...",
  "log_file": "iteration_3.log"
}
```

必要时再读取。

------

## 推荐增加

```cpp
bool outputTruncated;
QString outputFile;
```

------

# 3. Execution Result 非结构化

------

## 问题现象

当前：

```text
Operation SUCCESS ...
```

属于：

```text
自然语言反馈
```

LLM 很难稳定解析。

------

## 当前问题

AI 无法可靠判断：

- exit code
- stderr
- timeout
- 哪一步失败
- 是否部分成功

------

## 当前实现

```cpp
feedback += "Operation SUCCESS"
```

------

## 改进建议

必须改为：

```json
{
  "iteration": 2,
  "operations": [
    {
      "success": true,
      "exit_code": 0,
      "stdout": "...",
      "stderr": "...",
      "elapsed_ms": 1200
    }
  ]
}
```

------

## 推荐新增字段

| 字段           | 作用        |
| -------------- | ----------- |
| `success`      | 成功状态    |
| `exit_code`    | shell退出码 |
| `stdout`       | 标准输出    |
| `stderr`       | 错误输出    |
| `timeout`      | 是否超时    |
| `cancelled`    | 是否被取消  |
| `working_dir`  | 工作目录    |
| `operation_id` | 操作唯一ID  |

------

# 4. 缺少 Streaming IO

------

## 问题现象

长时间任务：

```bash
npm install
cargo build
ollama pull
```

期间：

- UI 卡死
- 无输出
- 用户以为崩溃
- AI无法观察过程

------

## 原因分析

当前大概率：

```cpp
waitForFinished()
```

阻塞式读取。

------

## 风险

Windows Pipe Buffer 满后：

```text
进程永久卡死
```

这是 QProcess 常见问题。

------

## 改进建议

------

## 使用 readyReadStandardOutput

```cpp
connect(process, &QProcess::readyReadStandardOutput, ...)
```

实时读取。

------

## 推荐新增

```cpp
stdoutChunkReceived()
stderrChunkReceived()
```

------

## UI 改进

支持：

- 实时 terminal
- 动态日志
- 中途取消
- 进度显示

------

# 5. Agent 没有 Memory

------

## 问题现象

AI：

- 重复 mkdir
- 重复 npm install
- 无限循环
- 忘记之前错误

------

## 原因分析

当前：

```text
每轮只喂上一轮结果
```

没有：

```text
持续上下文
```

------

## 改进建议

新增：

```cpp
struct AgentContext
```

例如：

```cpp
struct AgentContext {
    QString cwd;
    QStringList createdFiles;
    QStringList completedTasks;
    QStringList failedOperations;
    QString lastError;
};
```

------

## 推荐分层

| Memory 类型        | 内容         |
| ------------------ | ------------ |
| Working Memory     | 当前任务状态 |
| Episodic Memory    | 历史步骤     |
| Environment Memory | 文件系统状态 |
| Error Memory       | 已失败方案   |

------

# 6. 缺少 Persistent Working Directory

------

## 问题现象

AI：

```bash
cd project
npm install
```

下一轮：

```text
cwd 丢失
```

------

## 原因分析

workingDir 只是 operation 参数。

没有：

```text
Session-level cwd state
```

------

## 改进建议

AgentLoop 保存：

```cpp
QString m_currentWorkingDir;
```

每次：

```text
执行 cd
更新状态
```

后续 operation 默认继承。

------

# 7. 缺少 Retry / Recovery Strategy

------

## 问题现象

任何错误：

```text
直接失败
```

AI 不知道：

- 如何恢复
- 是否应该 retry
- 是否环境问题

------

## 改进建议

增加：

```cpp
enum FailureType
```

例如：

| 类型              | 示例     |
| ----------------- | -------- |
| Transient         | 网络错误 |
| Permission        | 权限问题 |
| MissingDependency | 缺少依赖 |
| SyntaxError       | 命令错误 |
| Timeout           | 超时     |

------

## 推荐加入

```text
Self-healing Strategy
```

例如：

- 自动 retry
- fallback shell
- 自动安装缺失依赖
- rollback

------

# 8. SafetyChecker 仍存在缺陷

------

## 当前优点

已经很好：

- 路径白名单
- 系统目录保护
- 注入检测
- Windows/Linux/macOS兼容

------

## 当前问题

仍属于：

```text
Regex-based security
```

容易绕过。

------

## 风险

例如：

```bash
python -c "..."
```

可能绕过。

PowerShell：

```powershell
&(gp variable:foo)
```

也可能绕过。

------

## 改进建议

------

## 不要仅靠 Regex

增加：

```text
AST级别命令解析
```

------

## 推荐方向

### Unix

使用：

```text
shell parser
```

### PowerShell

使用：

```text
PowerShell AST
```

------

## 更高级方案

真正 sandbox：

- Windows Job Object
- Linux namespace
- macOS sandbox-exec

------

# 9. Undo 系统不完整

------

## 当前优点

设计方向很好。

已经接近：

```text
Transactional Execution
```

------

## 当前问题

很多操作不可逆：

- rm
- overwrite
- shell script
- network side effect

------

## 改进建议

------

## 引入 Snapshot

例如：

```text
执行前：
自动备份文件
```

------

## 推荐：

```text
Workspace Snapshot System
```

类似：

- git stash
- filesystem overlay

------

# 10. 缺少 Tool Abstraction

------

## 当前问题

系统过度依赖：

```text
shell_command
```

导致：

- 不稳定
- 不可控
- 跨平台困难
- prompt依赖强

------

## 改进建议

改为：

```json
{
  "tool": "read_file",
  "args": {
    "path": "main.cpp"
  }
}
```

------

## 推荐 Tool 类型

| Tool        | 作用     |
| ----------- | -------- |
| read_file   | 读文件   |
| write_file  | 写文件   |
| list_dir    | 列目录   |
| grep        | 搜索     |
| run_process | 运行进程 |
| git_status  | git操作  |

------

## 好处

- 更安全
- 更稳定
- 更易调试
- 更适合 agent

------

# 三、系统成熟度评估

------

# 当前阶段

你现在系统属于：

```text
Agent Runtime Prototype
```

已经超出：

```text
普通AI聊天软件
```

范畴。

------

# 已经具备的高级特征

| 特性                   | 状态   |
| ---------------------- | ------ |
| Plan/Execute 分离      | 已完成 |
| Safety Layer           | 已完成 |
| Undo System            | 已完成 |
| Structured IR          | 已完成 |
| Cross-platform Runtime | 已完成 |
| Multi-step Framework   | 半完成 |

------

# 当前最核心缺陷

真正缺：

| 核心能力            | 状态 |
| ------------------- | ---- |
| Async Orchestration | 缺失 |
| Persistent State    | 缺失 |
| Structured Feedback | 缺失 |
| Streaming IO        | 缺失 |
| Runtime Memory      | 缺失 |
| Self-healing        | 缺失 |

------

# 四、建议的下一阶段架构

推荐升级为：

```text
Stateful Async Agent Runtime
```

------

# 推荐架构

```text
AgentLoop
    ↓
AgentOrchestrator
    ↓
LLMWorker
    ↓
TaskEngine
    ↓
ToolExecutor
```

------

# 推荐新增模块

| 模块              | 作用       |
| ----------------- | ---------- |
| AgentContext      | 持久上下文 |
| ToolRegistry      | Tool管理   |
| ExecutionJournal  | 全量日志   |
| StreamingTerminal | 实时输出   |
| RetryManager      | 自动恢复   |
| WorkspaceSnapshot | 回滚系统   |

------

# 五、优先级排序（最重要）

------

# P0（必须立即修）

## 1. 真正闭环 Agent Loop

否则不是 autonomous agent。

------

## 2. 删除 stdout 80字符限制

否则 AI 永远无法稳定工作。

------

## 3. 改成结构化结果

否则 LLM 不可靠。

------

# P1（强烈建议）

## 4. Streaming IO

解决卡死与长任务问题。

------

## 5. Persistent Context

解决 AI “失忆”。

------

## 6. Working Directory State

解决多步骤环境问题。

------

# P2（架构升级）

## 7. Tool-based Runtime

替代 shell-first。

------

## 8. 真正 Sandbox

提高安全性。

------

## 9. Snapshot Undo

提高可恢复性。

------

# 六、最终总结

当前系统：

```text
已经具备了 Agent Runtime 的基础骨架
```

尤其：

- OperationPlan
- SafetyChecker
- Undo
- AgentLoop

这些方向是正确的。

但目前：

```text
“像Agent”
```

还没有真正成为：

```text
“持续自治的 Agent Runtime”
```

真正阻碍系统工作的核心点只有三个：

| 问题         | 严重程度 |
| ------------ | -------- |
| Loop未闭环   | 致命     |
| 输出被截断   | 致命     |
| 无结构化结果 | 极严重   |

优先修复这三个问题后，系统会立刻从：

```text
“半自动命令执行器”
```

提升为：

```text
真正可持续运行的本地 AI Agent
```