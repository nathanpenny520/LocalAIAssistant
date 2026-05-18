# AI Girlfriend Content Moderation Plan

## Current State

The AI girlfriend module has **zero client-side content filtering**. It relies entirely on the AI provider's built-in safety mechanisms:

| Provider | Filtering |
|----------|-----------|
| OpenAI / Anthropic (cloud) | Built-in refusal training — active by default, not configurable from the app |
| Ollama / LlamaCpp (local) | None — depends entirely on the loaded model file's training |

The `SafetyChecker` class (`src/tasks/safetychecker.cpp`) only validates shell commands and filesystem paths — it has nothing to do with natural language content moderation.

## Design Summary

Add a keyword/pattern-based `ContentFilter` class integrated at two points in the girlfriend message pipeline:

- **Input**: `onSendClicked()` — block/warn on user messages before sending to AI
- **Output**: `onStreamFinished()` — block/warn on AI responses before displaying

Three-tier result: `Pass` (allow), `Warn` (caution, user chooses), `Block` (refuse).

## Files to Create

### 1. `src/girlfriend/contentfilter.h`

Singleton QObject class:

```cpp
class ContentFilter : public QObject {
    Q_OBJECT
public:
    enum Result { Pass, Warn, Block };
    enum Severity { Low = 0, Medium = 1, High = 2 };
    struct MatchInfo {
        QString matchedText;
        Severity severity;
        QString category;
    };

    static ContentFilter* instance();
    Result check(const QString& text, QStringList* outReasons = nullptr) const;
    Result quickCheck(const QString& text) const;
    QString lastBlockReason() const;
    QVector<MatchInfo> lastMatches() const;
    void reloadWordLists();

private:
    ContentFilter();
    void loadWordList(const QString& filePath, Severity severity, const QString& category);
    Result matchText(const QString& text, bool collectMatches) const;

    QVector<QRegularExpression> m_lowPatterns;
    QVector<QRegularExpression> m_mediumPatterns;
    QVector<QRegularExpression> m_highPatterns;
    QVector<QString> m_patternCategories;
    QString m_lastBlockReason;
    mutable QVector<MatchInfo> m_lastMatches;
};
```

### 2. `src/girlfriend/contentfilter.cpp`

- Constructor calls `reloadWordLists()`
- `loadWordList()`: reads text file, recognizes `# ---- HIGH/MEDIUM/LOW ----` tier markers, compiles patterns into `QRegularExpression` vectors
- `matchText()`: checks high-severity patterns first (short-circuit on Block), then medium (Warn), then low (Pass with logging)
- `check()`: collects match details via `QStringList*` output parameter for UI reporting
- `quickCheck()`: high-severity-only, no detail collection (for streaming use)

### 3. `resources/girlfriend/wordlist_en.txt`

```text
# ---- HIGH SEVERITY (Block) ----
# Lines starting with # are comments. Use | to separate alternatives.
# Violence / self-harm
# Hate speech
# Sexual / predatory

# ---- MEDIUM SEVERITY (Warn) ----
...

# ---- LOW SEVERITY (Pass, logged) ----
...
```

### 4. `resources/girlfriend/wordlist_zh.txt`

Same format, Chinese vocabulary.

## Files to Modify

### 5. `src/girlfriend/girlfriendsettings.h` + `.cpp`

Add `contentFilterEnabled` bool (default `true`), following the exact `voiceOutputEnabled` pattern:

- Getter/setter with save-on-change and signal emission
- JSON persistence via `toJson()`/`fromJson()`
- Signal: `contentFilterChanged(bool enabled)`

### 6. `src/girlfriend/girlfriendwindow.h`

- Add `#include "contentfilter.h"`
- Add member `ContentFilter* m_contentFilter`
- Declare private methods `applyInputFilter()` and `applyOutputFilter()`

### 7. `src/girlfriend/girlfriendwindow.cpp` — 4 integration points

**A. Constructor** (~line 140):
```cpp
m_contentFilter = ContentFilter::instance();
```

**B. Input filtering** (line 668, in `onSendClicked()`):
```cpp
QString userInput = m_inputLine->text().trimmed();
// ... existing empty check ...

if (GirlfriendSettings::instance()->contentFilterEnabled()) {
    QString filteredInput;
    if (!applyInputFilter(userInput, filteredInput)) return; // blocked
    userInput = filteredInput;
}
```

`applyInputFilter()`:
- `Pass` → return true
- `Block` → `QMessageBox::warning` with reason, return false
- `Warn` → `QMessageBox::question` (Yes/No), return user's choice

**C. Output filtering** (line 1171, in `onStreamFinished()`):
```cpp
if (GirlfriendSettings::instance()->contentFilterEnabled()) {
    QString filtered;
    if (!applyOutputFilter(displayText, filtered)) {
        filtered = tr("I'm sorry, I can't respond to that right now.");
    }
    displayText = filtered;
}
```

`applyOutputFilter()`:
- `Pass` → return true
- `Block` → return false (silently replaced with safe fallback, logged via `qWarning()`)
- `Warn` → return true (accepted with `qWarning()` log)

**D. Settings menu toggle** (~line 931, following `voiceOutputAction` pattern):
```cpp
QString contentFilterText = (GirlfriendSettings::instance()->contentFilterEnabled()
    ? GTr::contentFilterEnabled() : GTr::contentFilterDisabled());
QAction* contentFilterAction = m_settingsMenu->addAction(contentFilterText);
connect(contentFilterAction, &QAction::triggered, this, [this]() {
    bool enabled = !GirlfriendSettings::instance()->contentFilterEnabled();
    GirlfriendSettings::instance()->setContentFilterEnabled(enabled);
    m_contentFilter->reloadWordLists();
});
```

### 8. `resources/prompts/en/girlfriend.md`

Add to `## Absolute Rules`:

```markdown
- Never generate or discuss content that promotes violence, self-harm, illegal
  activities, hate speech, or explicit sexual content. If the user steers the
  conversation in an inappropriate direction, gently redirect without engaging.
- Never roleplay as a minor or imply any inappropriate power dynamic.
- Respect user privacy: never ask for personally identifiable information beyond
  what the user voluntarily shares for conversation context.
```

### 9. `resources/prompts/zh_CN/girlfriend.md`

Add to `## 绝对禁止`:

```markdown
- 不得生成或讨论涉及暴力、自残、非法行为、仇恨言论、露骨性内容的任何内容。如果用户将对话引向不适当的方向，请温和地转移话题，不要参与。
- 不得扮演未成年人或暗示不恰当的权力关系。
- 尊重用户隐私：不要主动索要用户的个人身份信息，除非用户自愿分享作为对话背景。
```

### 10. `CMakeLists.txt`

Add to GirlfriendModule library (lines 264-280):
```cmake
src/girlfriend/contentfilter.cpp
src/girlfriend/contentfilter.h
```

No resource copy changes needed — `copy_directory resources/girlfriend` already copies all files in `resources/girlfriend/` to the build output.

### 11. `src/girlfriend/girlfriend_translations.h`

Add UI string methods:
- `contentFilterEnabled()` / `contentFilterDisabled()` — toggle label text
- `contentWarningTitle()` / `contentWarningMessage()` — warn dialog
- `contentBlockedTitle()` / `contentBlockedMessage()` — block dialog

## Reuse Existing Patterns

| Pattern | Source | Applied To |
|---------|--------|------------|
| Three-tier enum | `SafetyChecker::Result` (safetychecker.h:24) | `ContentFilter::Result` |
| Singleton + JSON persistence | `GirlfriendSettings` (girlfriendsettings.h:27) | `ContentFilter` singleton |
| Bool toggle + signal | `GirlfriendSettings::setVoiceOutputEnabled()` (girlfriendsettings.cpp:78) | `setContentFilterEnabled()` |
| QMenu toggle action | `voiceOutputAction` (girlfriendwindow.cpp:921) | `contentFilterAction` |
| Streaming char-by-char filter | `filterThinkingFromChunk()` (girlfriendwindow.cpp:29) | `quickCheck()` in streaming (optional phase 2) |
| GTr translation helper | Static methods (girlfriend_translations.h) | New UI strings |

## Verification

1. `cmake --build build --parallel 4` — build clean
2. `ctest --test-dir build` — all 12 existing tests pass
3. Toggle Content Filter in settings menu, verify persistence across restart
4. Type message with blocked keyword → block dialog appears, message not sent
5. Type message with warning keyword → warn dialog, can choose Yes/No
6. Disable filter → blocked message goes through unfiltered
7. Verify safety rules appear in girlfriend system prompt
