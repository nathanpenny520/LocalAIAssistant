# 超拟人语音合成 API 升级 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 VoiceManager 从在线语音合成 API 升级到超拟人语音合成 API，实现流式输入、口语化和更自然的语音输出。

**Architecture:** 保持 VoiceManager 的现有架构，替换 TTS WebSocket 协议实现。新增流式文本输入支持，允许在 AI 流式回复时实时合成语音。协议结构从 `{common, business, data}` 改为 `{header, parameter, payload}`。

**Tech Stack:** Qt 6 WebSocket, QMediaPlayer, 讯飞超拟人 TTS API

---

## File Structure

| 文件 | 责任 | 操作 |
|-----|-----|-----|
| `src/girlfriend/voicemanager.h` | VoiceManager 类定义 | 修改：添加流式 TTS 相关成员变量 |
| `src/girlfriend/voicemanager.cpp` | TTS 实现 | 修改：重写 TTS 协议逻辑 |
| `src/girlfriend/girlfriendwindow.cpp` | 调用 VoiceManager | 修改：支持流式 TTS 调用 |
| `.env` | API 配置 | 修改：更新 TTS URL |
| `.env.example` | 配置示例 | 修改：更新示例 |

---

### Task 1: 更新 API 地址和常量定义

**Files:**
- Modify: `src/girlfriend/voicemanager.cpp:24-28`

- [ ] **Step 1: 更新 TTS API 地址常量**

修改 `voicemanager.cpp` 第 24-28 行：

```cpp
// 超拟人语音合成 WebSocket API 鉴权参数
static const QString ASR_HOST = "iat-api.xfyun.cn";
static const QString ASR_PATH = "/v2/iat";
static const QString TTS_HOST = "cbm01.cn-huabei-1.xf-yun.com";
static const QString TTS_PATH = "/v1/private/mcd9m97e6";
```

- [ ] **Step 2: 更新默认 TTS URL 配置**

修改 `voicemanager.cpp` 第 165 行：

```cpp
m_ttsUrl = qEnvironmentVariable("XFYUN_TTS_URL", "wss://cbm01.cn-huabei-1.xf-yun.com/v1/private/mcd9m97e6");
```

- [ ] **Step 3: 编译验证常量更新**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh --no-run`
Expected: Build succeeded

- [ ] **Step 4: 提交常量更新**

```bash
git add src/girlfriend/voicemanager.cpp
git commit -m "feat(tts): update API endpoint to super realistic TTS"
```

---

### Task 2: 添加流式 TTS 相关成员变量

**Files:**
- Modify: `src/girlfriend/voicemanager.h:159-166`

- [ ] **Step 1: 添加流式 TTS 状态变量**

在 `voicemanager.h` 的 private 成员区域（约第 159-166 行后）添加：

```cpp
// 流式 TTS 状态
int m_ttsSeq;                   // 流式文本序号
bool m_ttsStreaming;            // 是否正在流式合成
QString m_ttsStreamingBuffer;   // 流式文本缓冲区
```

- [ ] **Step 2: 初始化新成员变量**

修改 `voicemanager.cpp` 构造函数（约第 46 行后）添加初始化：

```cpp
, m_ttsSeq(0)
, m_ttsStreaming(false)
, m_ttsStreamingBuffer("")
```

- [ ] **Step 3: 编译验证成员变量添加**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh --no-run`
Expected: Build succeeded

- [ ] **Step 4: 提交成员变量添加**

```bash
git add src/girlfriend/voicemanager.h src/girlfriend/voicemanager.cpp
git commit -m "feat(tts): add streaming TTS state variables"
```

---

### Task 3: 重写 TTS 请求协议结构

**Files:**
- Modify: `src/girlfriend/voicemanager.cpp:936-977`

- [ ] **Step 1: 重写 sendTtsRequest 函数**

完全替换 `sendTtsRequest` 函数（第 936-977 行）：

```cpp
void VoiceManager::sendTtsRequest(const QString &text)
{
    if (!m_ttsConnected || text.isEmpty()) {
        return;
    }

    // 超拟人 TTS 协议结构
    QJsonObject frame;

    // header 协议头
    QJsonObject header;
    header["app_id"] = m_appId;
    header["status"] = 2;  // 一次性合成传 2，流式传 0/1/2
    frame["header"] = header;

    // parameter 能力参数
    QJsonObject parameter;

    // oral 口语化配置（仅 x4 系列发音人支持）
    QJsonObject oral;
    oral["oral_level"] = "mid";      // 口语化等级：high/mid/low
    oral["spark_assist"] = 1;        // 大模型辅助口语化
    oral["stop_split"] = 0;          // 不关闭服务端拆句
    oral["remain"] = 0;              // 不保留原书面语
    parameter["oral"] = oral;

    // tts 合成参数
    QJsonObject tts;
    tts["vcn"] = m_voiceType;        // 发音人：x6_wumeinv_pro
    tts["speed"] = 50;               // 语速 0-100
    tts["volume"] = 50;              // 音量 0-100
    tts["pitch"] = 50;               // 语调 0-100
    tts["bgs"] = 0;                  // 无背景音
    tts["reg"] = 0;                  // 英文发音方式：自动判断
    tts["rdn"] = 0;                  // 数字发音方式：自动判断
    tts["rhy"] = 0;                  // 不返回拼音标注

    // audio 音频格式参数
    QJsonObject audio;
    audio["encoding"] = "lame";      // MP3 格式
    audio["sample_rate"] = 24000;    // 24k 采样率（超拟人支持更高音质）
    audio["channels"] = 1;           // 单声道
    audio["bit_depth"] = 16;         // 16 bit
    audio["frame_size"] = 0;         // 默认帧大小
    tts["audio"] = audio;

    parameter["tts"] = tts;
    frame["parameter"] = parameter;

    // payload 输入数据
    QJsonObject payload;
    QJsonObject textObj;
    textObj["encoding"] = "utf8";
    textObj["compress"] = "raw";
    textObj["format"] = "plain";
    textObj["status"] = 2;           // 数据状态：2=结束
    textObj["seq"] = 0;              // 序号
    textObj["text"] = QString(text.toUtf8().toBase64());
    payload["text"] = textObj;
    frame["payload"] = payload;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent super realistic TTS request";
    qDebug() << "  - Text:" << text;
    qDebug() << "  - Voice:" << m_voiceType;
}
```

- [ ] **Step 2: 编译验证协议重写**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh --no-run`
Expected: Build succeeded

- [ ] **Step 3: 提交协议结构重写**

```bash
git add src/girlfriend/voicemanager.cpp
git commit -m "feat(tts): rewrite request protocol for super realistic TTS API"
```

---

### Task 4: 重写 TTS 响应解析

**Files:**
- Modify: `src/girlfriend/voicemanager.cpp:882-926`

- [ ] **Step 1: 重写 onTtsTextMessageReceived 函数**

完全替换 `onTtsTextMessageReceived` 函数（第 882-926 行）：

```cpp
void VoiceManager::onTtsTextMessageReceived(const QString &message)
{
    // 解析超拟人 TTS JSON 响应
    qDebug() << "VoiceManager: TTS received text message:" << message.left(100);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "VoiceManager: TTS response is not JSON object";
        return;
    }

    QJsonObject response = doc.object();

    // 解析 header
    QJsonObject header = response["header"].toObject();
    int code = header["code"].toInt();
    if (code != 0) {
        QString errorMsg = header["message"].toString();
        emit ttsError(GTr::ttsErrorWithCode(code, errorMsg));
        return;
    }

    // 解析 payload
    QJsonObject payload = response["payload"].toObject();
    if (payload.isEmpty()) {
        // 空数据帧，忽略（文档说明：data 为空且 code=0 可直接忽略）
        return;
    }

    // 解析 audio
    QJsonObject audioObj = payload["audio"].toObject();
    QString audioBase64 = audioObj["audio"].toString();

    if (!audioBase64.isEmpty()) {
        // 解码 base64 并追加到音频缓冲区
        QByteArray audioData = QByteArray::fromBase64(audioBase64.toUtf8());
        m_ttsAudioBuffer.append(audioData);
        qDebug() << "VoiceManager: TTS decoded audio:" << audioData.size()
                 << "bytes, total buffer:" << m_ttsAudioBuffer.size();
    }

    // 检查合成状态
    int status = audioObj["status"].toInt();
    if (status == 2) {
        // 合成完成，播放音频
        qDebug() << "VoiceManager: TTS synthesis complete, total audio:"
                 << m_ttsAudioBuffer.size() << "bytes";
        emit statusChanged(GTr::voiceSynthesisComplete());

        if (!m_ttsAudioBuffer.isEmpty()) {
            QByteArray audioToPlay = m_ttsAudioBuffer;
            m_ttsAudioBuffer.clear();
            playTtsAudio(audioToPlay);
        }
    }
}
```

- [ ] **Step 2: 编译验证响应解析重写**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh --no-run`
Expected: Build succeeded

- [ ] **Step 3: 提交响应解析重写**

```bash
git add src/girlfriend/voicemanager.cpp
git commit -m "feat(tts): rewrite response parsing for super realistic TTS API"
```

---

### Task 5: 添加流式 TTS 公共方法

**Files:**
- Modify: `src/girlfriend/voicemanager.h:46-49`
- Modify: `src/girlfriend/voicemanager.cpp`

- [ ] **Step 1: 在头文件添加流式 TTS 方法声明**

在 `voicemanager.h` 的 public 方法区域（约第 46-49 行后）添加：

```cpp
// 流式 TTS：分段发送文本
void startStreamingTts();
void sendStreamingText(const QString &text);
void finishStreamingTts();
bool isStreamingTts() const { return m_ttsStreaming; }
```

- [ ] **Step 2: 实现流式 TTS 方法**

在 `voicemanager.cpp` 的 TTS 区域添加以下函数：

```cpp
void VoiceManager::startStreamingTts()
{
    if (!isConfigured()) {
        emit ttsError(GTr::xunfeiCredentialsNotConfigured());
        return;
    }

    m_ttsAudioBuffer.clear();
    m_ttsSeq = 0;
    m_ttsStreaming = true;

    emit statusChanged(GTr::synthesizingVoice());

    // 连接 TTS WebSocket
    QString authUrl = generateTtsAuthUrl();
    qDebug() << "VoiceManager: Connecting to streaming TTS:" << authUrl.left(100) + "...";
    m_ttsWebSocket->open(QUrl(authUrl));
}

void VoiceManager::sendStreamingText(const QString &text)
{
    if (!m_ttsConnected || text.isEmpty() || !m_ttsStreaming) {
        return;
    }

    m_ttsSeq++;

    QJsonObject frame;

    // header
    QJsonObject header;
    header["app_id"] = m_appId;
    header["status"] = 1;  // 流式中间帧
    frame["header"] = header;

    // parameter (首次发送后后续帧可省略，但为安全每次发送)
    QJsonObject parameter;
    QJsonObject oral;
    oral["oral_level"] = "mid";
    oral["spark_assist"] = 1;
    parameter["oral"] = oral;

    QJsonObject tts;
    tts["vcn"] = m_voiceType;
    QJsonObject audio;
    audio["encoding"] = "lame";
    audio["sample_rate"] = 24000;
    audio["channels"] = 1;
    audio["bit_depth"] = 16;
    audio["frame_size"] = 0;
    tts["audio"] = audio;
    parameter["tts"] = tts;
    frame["parameter"] = parameter;

    // payload
    QJsonObject payload;
    QJsonObject textObj;
    textObj["encoding"] = "utf8";
    textObj["compress"] = "raw";
    textObj["format"] = "plain";
    textObj["status"] = 1;  // 中间数据
    textObj["seq"] = m_ttsSeq;
    textObj["text"] = QString(text.toUtf8().toBase64());
    payload["text"] = textObj;
    frame["payload"] = payload;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent streaming TTS text seq=" << m_ttsSeq
             << "text=" << text.left(20) << "...";
}

void VoiceManager::finishStreamingTts()
{
    if (!m_ttsConnected || !m_ttsStreaming) {
        return;
    }

    m_ttsStreaming = false;

    // 发送结束帧
    QJsonObject frame;

    QJsonObject header;
    header["app_id"] = m_appId;
    header["status"] = 2;  // 结束
    frame["header"] = header;

    QJsonObject payload;
    QJsonObject textObj;
    textObj["encoding"] = "utf8";
    textObj["compress"] = "raw";
    textObj["format"] = "plain";
    textObj["status"] = 2;  // 结束数据
    textObj["seq"] = m_ttsSeq + 1;
    textObj["text"] = "";
    payload["text"] = textObj;
    frame["payload"] = payload;

    QString jsonFrame = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    m_ttsWebSocket->sendTextMessage(jsonFrame);

    qDebug() << "VoiceManager: Sent streaming TTS end signal";
}
```

- [ ] **Step 3: 编译验证流式方法**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh --no-run`
Expected: Build succeeded

- [ ] **Step 4: 提交流式 TTS 方法**

```bash
git add src/girlfriend/voicemanager.h src/girlfriend/voicemanager.cpp
git commit -m "feat(tts): add streaming TTS methods for real-time synthesis"
```

---

### Task 6: 更新默认发音人配置

**Files:**
- Modify: `src/girlfriend/voicemanager.cpp:44`

- [ ] **Step 1: 更新默认发音人为超拟人系列**

修改 `voicemanager.cpp` 第 44 行：

```cpp
, m_voiceType("x6_wumeinv_pro")  // 超拟人默认发音人
```

- [ ] **Step 2: 编译验证发音人更新**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh --no-run`
Expected: Build succeeded

- [ ] **Step 3: 提交发音人更新**

```bash
git add src/girlfriend/voicemanager.cpp
git commit -m "feat(tts): update default voice to super realistic series"
```

---

### Task 7: 更新配置文件示例

**Files:**
- Modify: `.env.example`

- [ ] **Step 1: 更新 .env.example 中的 TTS URL**

读取并更新 `.env.example` 文件，将 TTS URL 改为超拟人 API：

```bash
# 讯飞语音服务凭证
XFYUN_APP_ID=your_app_id
XFYUN_API_KEY=your_api_key
XFYUN_API_SECRET=your_api_secret

# ASR (语音识别) - 在线语音听写
XFYUN_ASR_URL=wss://iat-api.xfyun.cn/v2/iat

# TTS (语音合成) - 超拟人语音合成
XFYUN_TTS_URL=wss://cbm01.cn-huabei-1.xf-yun.com/v1/private/mcd9m97e6

# 发音人（超拟人系列）
XFYUN_VOICE_TYPE=x6_wumeinv_pro
```

- [ ] **Step 2: 提交配置示例更新**

```bash
git add .env.example
git commit -m "docs: update .env.example for super realistic TTS"
```

---

### Task 8: 更新 playTtsAudio 音频格式处理

**Files:**
- Modify: `src/girlfriend/voicemanager.cpp:1007-1067`

- [ ] **Step 1: 更新 playTtsAudio 注释**

修改 `playTtsAudio` 函数开头注释（第 1015 行）：

```cpp
// 超拟人 TTS 返回的是 MP3 格式音频（encoding=lame），24k 采样率
QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
QString tempFile = tempPath + "/tts_output_" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".mp3";
```

- [ ] **Step 2: 编译验证**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh --no-run`
Expected: Build succeeded

- [ ] **Step 3: 提交音频格式注释更新**

```bash
git add src/girlfriend/voicemanager.cpp
git commit -m "docs(tts): update audio format comments for 24k sample rate"
```

---

### Task 9: 功能验证测试

**Files:**
- Test: Manual testing via GUI application

- [ ] **Step 1: 编译最终版本**

Run: `cd /Users/nathanpenny/Projects/locai/sourcecode-ai-assistant && ./scripts/build.sh`
Expected: Build succeeded

- [ ] **Step 2: 启动应用程序进行手动测试**

Run GUI application and test:
1. 打开 AI Girlfriend 窗口
2. 发送一条消息触发 AI 回复
3. 观察 TTS 语音输出是否正常
4. 检查日志输出确认使用超拟人 API

Expected: 
- 语音正常播放
- 日志显示 "Sent super realistic TTS request"
- 音质比之前更自然

- [ ] **Step 3: 测试发音人切换**

在设置中或代码中测试不同发音人：
- `x6_wumeinv_pro` (默认)
- `x5_lingfeiyi_flow` (男声)
- `x6_lingxiaoli_pro` (更高质量)

Expected: 不同发音人声音正常

- [ ] **Step 4: 测试错误处理**

测试以下错误场景：
1. 无网络连接
2. API 凭证错误
3. 未授权发音人

Expected: 正确显示错误信息

---

### Task 10: 最终提交和文档

- [ ] **Step 1: 确认所有修改已提交**

Run: `git status`
Expected: No uncommitted changes

- [ ] **Step 2: 查看提交历史**

Run: `git log --oneline -10`
Expected: 看到 6-8 个本次升级相关提交

- [ ] **Step 3: 创建升级总结提交**

```bash
git log --oneline HEAD~8..HEAD > /tmp/upgrade_log.txt
echo "超拟人语音合成 API 升级完成" >> /tmp/upgrade_log.txt
```

---

## Self-Review Checklist

**1. Spec coverage:**
- ✅ API 地址更新 - Task 1
- ✅ 协议结构 `{header, parameter, payload}` - Task 3
- ✅ 口语化参数 `oral_level` - Task 3
- ✅ 音频格式 `encoding=lame, sample_rate=24000` - Task 3
- ✅ 流式输入支持 - Task 5
- ✅ 响应解析 - Task 4
- ✅ 发音人配置 - Task 6
- ✅ 配置文件示例 - Task 7

**2. Placeholder scan:**
- ✅ 无 "TBD", "TODO", "implement later"
- ✅ 无 "Add appropriate error handling"
- ✅ 所有代码步骤包含完整代码

**3. Type consistency:**
- ✅ `m_ttsSeq` 为 int
- ✅ `m_ttsStreaming` 为 bool
- ✅ `m_ttsStreamingBuffer` 为 QString
- ✅ 所有 JSON 字段名称与文档一致