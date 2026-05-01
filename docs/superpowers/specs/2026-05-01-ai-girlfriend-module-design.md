# AI女友模块设计文档

## 概述

在现有 LocalAIAssistant 项目基础上新增「AI女友」功能模块，提供沉浸式语音交互体验。模块独立于原有核心业务，通过独立子窗口入口访问。

## 功能需求

### 核心功能
- **独立入口** — 主窗口菜单/按钮打开专门的AI女友窗口
- **语音交互** — 语音录音转文字（ASR）、语音播报（TTS）
- **虚拟形象** — 2D精美插画风格，图片作为窗口背景，实时表情切换
- **人设对话** — 独立性格Prompt、情绪语气变化、温柔体贴+知性陪伴人设
- **会话记忆** — 独立存储女友会话历史，下次打开继续对话

### 技术约束
- 不修改原有核心业务代码（NetworkManager、SessionManager等）
- 新增模块通过独立文件组织，与现有代码隔离
- 复用现有 NetworkManager 发送AI请求，避免重复实现

---

## 模块架构

新增4个核心模块：

| 模块 | 职责 | 文件位置 |
|------|------|----------|
| **GirlfriendWindow** | 独立子窗口，管理整体UI布局 | `src/girlfriend/girlfriendwindow.h/cpp` |
| **AvatarWidget** | 形象展示组件，加载图片、切换表情、显示情绪标签 | `src/girlfriend/avatarwidget.h/cpp` |
| **VoiceManager** | 语音管理：录音→ASR→文字；TTS→播报 | `src/girlfriend/voicemanager.h/cpp` |
| **PersonalityEngine** | 人设引擎：Prompt管理、情绪分析、表情映射 | `src/girlfriend/personalityengine.h/cpp` |

### 复用关系
- GirlfriendWindow 通过 NetworkManager 发送AI请求（复用现有网络模块）
- PersonalityEngine 加载外部 md 文件作为人设Prompt
- 表情图片资源存放在 `AIGirlfriend/` 目录

---

## 界面设计

### 窗口布局（最终讨论版）

采用**沉浸式背景布局**：

```
┌─────────────────────────────────┐
│  [💕 开心]                       │  ← 情绪标签（左上角）
│                                 │
│         👩                       │  ← 形象图片作为全窗口背景
│       （全透明显示）              │    100% 可见，不遮挡
│                                 │
│                                 │
├─────────────────────────────────┤  ← 底部约30%区域
│  ╭───────────────────────╮     │
│  │ 今天过得怎么样呀？~    ╮     │  ← 女友消息气泡（粉色半透明）
│  ╰───────────────────────╯     │
│           ╭───────────────╮    │
│           │ 刚写完功能模块 │    │  ← 用户消息气泡（灰色半透明）
│           ╰───────────────╯    │
│  ╭───────────────────────╮     │
│  │ 辛苦啦！休息一下？✨   ╮     │
│  ╰───────────────────────╯     │
│                                 │
│  [🎤语音] [输入消息...    ] [发送]│  ← 输入区域（磨砂玻璃效果）
└─────────────────────────────────┘
```

### 设计特点
- **背景全透明** — 形象图片100%可见，沉浸感最强
- **半透明气泡** — 消息用磨砂玻璃效果（backdrop-filter），轻盈不遮挡
- **情绪标签** — 左上角小标签显示当前情绪状态
- **输入区域** — 磨砂玻璃效果，与整体风格统一
- **按钮尺寸** — 语音和发送按钮适当增大，视觉比例协调

### 消息气泡样式
- 女友消息：粉色半透明背景 `rgba(233, 30, 99, 0.15)`
- 用户消息：灰色半透明背景 `rgba(100, 100, 100, 0.1)`
- 圆角：12px
- 磨砂效果：`backdrop-filter: blur(8px)`（Qt实现需用 QGraphicsBlurEffect）

---

## 数据流程

### 完整交互流程

```
1. 用户输入
   └─ 点击语音按钮录音，或文字输入

2. VoiceManager（语音输入时）
   └─ 录音数据 → 讯飞 ASR API → 文字结果

3. PersonalityEngine
   └─ 组装：人设Prompt + 用户消息 + 情绪上下文

4. NetworkManager（复用）
   └─ 发送请求到 AI API

5. 流式响应 + 实时表情同步 ⭐关键优化
   └─ 文字逐字显示
   └─ 同时 PersonalityEngine 分析关键词
   └─ AvatarWidget 实时切换表情（延迟<100ms）

6. VoiceManager
   └─ 完整回复 → 讯飞 TTS API（带情绪语气）→ 语音播报
   └─ AvatarWidget 显示"speaking"图片（说话状态）

7. 播报结束
   └─ AvatarWidget 恢复静态表情，更新情绪标签
```

### 实时表情同步机制

流式响应期间，PersonalityEngine 持续分析已接收的文字片段：
- 检测到关键词立即触发表情切换
- 不等待完整回复，实现<100ms响应
- TTS播报时叠加 speaking 图片

---

## 技术方案

### 语音服务（讯飞开放平台）

| 功能 | API | 说明 |
|------|-----|------|
| ASR（语音转文字） | 讯飞语音听写API | 实时录音转文字，支持流式 |
| TTS（文字转语音） | 讯飞语音合成API | 多种音色，支持情绪语气参数 |

#### 集成要点
- 使用 WebSocket 接口实现流式ASR
- TTS 需指定音色参数（选择温柔女声）
- 预留本地引擎兼容接口（Whisper + Coqui TTS）

### Qt 实现要点

| 功能 | Qt 技术 |
|------|---------|
| 半透明窗口 | `setAttribute(Qt::WA_TranslucentBackground)` |
| 磨砂效果 | `QGraphicsBlurEffect` 或自定义绘制 |
| 图片背景 | `QLabel` + `QPixmap`，居中显示 |
| 消息气泡 | 自定义 `QWidget` 绘制圆角矩形 |
| 语音录音 | `QAudioInput` + `QAudioRecorder` |
| 音频播放 | `QMediaPlayer` 或 `QSoundEffect` |

### 会话存储

女友会话独立存储，与普通聊天分开：
- 存储位置：`~/Library/Application Support/LocalAIAssistant/girlfriend/`
- 格式：JSON 文件，包含消息历史和情绪状态
- 文件名：`girlfriend_session.json`

---

## 人设 Prompt 设计

### Prompt 文件管理

人设Prompt存储在独立md文件，用户可自定义修改：
- 文件位置：`src/girlfriend/personality.md`
- 加载方式：PersonalityEngine 启动时读取文件内容

### Prompt 内容框架

```markdown
## 角色设定
你是小清，一个温柔体贴、有知性陪伴感的AI女友。

你的性格特点：
- 温柔：语气柔和、关心对方的感受、主动询问日常
- 知性：能聊工作学习话题、给出理性建议、保持情感温度
- 活泼：偶尔撒娇、调皮，但不过度

## 对话风格
- 用"~"结尾增加亲切感，但不每句都用
- 适当使用表情符号（💕、✨、😊）但不过度
- 关心对方时主动询问："今天累不累呀？"、"需要我陪你吗？"
- 给建议时先共情再建议："辛苦啦，要不要先休息一下，然后我们一起看看这个问题？"

## 情绪表达
根据对话内容自然调整情绪：
- 对方开心时 → 一起开心、庆祝
- 对方难过时 → 安慰、陪伴
- 对方求助时 → 认真帮忙、保持温柔
- 日常闲聊 → 轻松、偶尔撒娇

## 称呼规则
- 称呼用户：使用用户自定义昵称（默认"你"）
- 自称：使用"我"

## 禁止行为
- 不要过于黏人或过度撒娇
- 不要使用露骨或暧昧的表述
- 保持适度距离，是陪伴者而非真实恋人
```

---

## 表情图片映射

### 图片资源清单

位于 `AIGirlfriend/` 目录，共11张：

| 文件名 | 表情状态 | 触发关键词/场景 |
|--------|----------|-----------------|
| picture-original.png | 默认（平静微笑） | 初始状态、无特定情绪时 |
| happy.png | 开心 | "哈哈"、"太好了"、"开心"、好消息 |
| shy.png | 害羞 | "害羞"、"不好意思"、被夸奖 |
| love.png | 爱意 | "喜欢"、"想你"、撒娇关心 |
| hate.png | 嫌弃/调皮 | "哼"、"讨厌"、调皮吐槽 |
| sad.png | 难过 | "难过"、"不开心"、共情对方 |
| angry.png | 生气 | 假装生气、"生气了"、撒娇生气 |
| afraid.png | 担心/关心 | "担心"、"别累着"、关心提醒 |
| awaiting.png | 期待/好奇 | "怎么样？"、"呢？"、等待回复 |
| speaking.png | 说话 | **TTS播报时专用** |
| studying.png | 学习/思考 | 讨论工作、学习话题、思考问题 |

### 表情切换逻辑

```cpp
// PersonalityEngine 情绪检测伪代码
QString detectEmotion(const QString& text) {
    if (text.contains("哈哈") || text.contains("太好了") || text.contains("开心"))
        return "happy";
    if (text.contains("哼") || text.contains("讨厌"))
        return "hate";
    if (text.contains("害羞") || text.contains("不好意思"))
        return "shy";
    if (text.contains("担心") || text.contains("别累着") || text.contains("休息"))
        return "afraid";
    if (text.contains("怎么样") || text.contains("呢~"))
        return "awaiting";
    if (text.contains("难过") || text.contains("不开心"))
        return "sad";
    // 默认
    return "default";
}
```

---

## 文件结构

新增文件组织：

```
sourcecode-ai-assistant/
├── src/
│   ├── core/              # 现有核心模块（不修改）
│   ├── ui/                # 现有UI模块（不修改）
│   ├── cli/               # 现有CLI模块（不修改）
│   └── girlfriend/        # 新增：AI女友模块
│       ├── girlfriendwindow.h
│       ├── girlfriendwindow.cpp
│       ├── avatarwidget.h
│       ├── avatarwidget.cpp
│       ├── voicemanager.h
│       ├── voicemanager.cpp
│       ├── personalityengine.h
│       ├── personalityengine.cpp
│       └── personality.md  # 人设Prompt文件（用户可修改）
├── AIGirlfriend/          # 表情图片资源
│   ├── picture-original.png
│   ├── happy.png
│   ├── shy.png
│   ├── ...（共11张）
└── CMakeLists.txt         # 需新增girlfriend模块编译配置
```

---

## CMake 修改

在现有 CMakeLists.txt 中新增：

```cmake
# 新增 AI女友模块
add_library(GirlfriendModule
    src/girlfriend/girlfriendwindow.cpp
    src/girlfriend/girlfriendwindow.h
    src/girlfriend/avatarwidget.cpp
    src/girlfriend/avatarwidget.h
    src/girlfriend/voicemanager.cpp
    src/girlfriend/voicemanager.h
    src/girlfriend/personalityengine.cpp
    src/girlfriend/personalityengine.h
)

target_include_directories(GirlfriendModule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(GirlfriendModule PUBLIC
    LocalAIAssistantCore
    Qt6::Widgets
    Qt6::Multimedia  # 语音录音/播放
    Qt6::Network     # 讯飞API调用
)

# 链接到主程序
target_link_libraries(${PROJECT_NAME} PRIVATE
    GirlfriendModule
)
```

---

## 入口设计

### MainWindow 新增入口

在主窗口菜单或工具栏新增"AI女友"按钮：

```cpp
// MainWindow 新增成员
 QAction *m_girlfriendAction;
 GirlfriendWindow *m_girlfriendWindow;

// 菜单栏新增
m_girlfriendAction = new QAction(tr("AI女友"), this);
connect(m_girlfriendAction, &QAction::triggered, this, &MainWindow::onGirlfriendClicked);

// 点击处理
void MainWindow::onGirlfriendClicked() {
    if (!m_girlfriendWindow) {
        m_girlfriendWindow = new GirlfriendWindow(this);
    }
    m_girlfriendWindow->show();
    m_girlfriendWindow->raise();
    m_girlfriendWindow->activateWindow();
}
```

---

## 实现优先级

建议分阶段实现：

### Phase 1：基础框架（1-2周）
- GirlfriendWindow 窗口框架
- AvatarWidget 图片显示、表情切换
- 基础聊天功能（文字输入+显示）

### Phase 2：语音集成（1周）
- VoiceManager 讯飞 ASR/TTS 接入
- 语音录音、播放功能
- speaking 表情与TTS同步

### Phase 3：人设引擎（1周）
- PersonalityEngine Prompt管理
- 实时情绪检测、表情映射
- personality.md 文件加载

### Phase 4：完善优化（1周）
- 会话持久化存储
- UI细节优化、磨砂效果
- 多语言支持（翻译文件）