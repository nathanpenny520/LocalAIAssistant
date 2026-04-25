# AI 模型参数配置扩展设计

## 概述

扩展 LocalAIAssistant CLI 的模型参数配置能力，支持完整的 OpenAI 格式参数，包括新增 `top_p`、`frequency_penalty`、`seed` 参数，并修复现有参数发送时硬编码的问题。

## 参数列表

| 参数 | 类型 | 默认值 | 范围 | CLI 选项 |
|------|------|--------|------|----------|
| `temperature` | double | 0.4 | 0.0 - 2.0 | `--temperature` |
| `top_p` | double | 1.0 | 0.0 - 1.0 | `--top-p` |
| `max_tokens` | int | 8192 | 1 - 128000 | `--max-tokens` |
| `presence_penalty` | double | 0.2 | -2.0 - 2.0 | `--presence-penalty` |
| `frequency_penalty` | double | 0.0 | -2.0 - 2.0 | `--frequency-penalty` |
| `seed` | int | null | 任意整数 | `--seed` |
| `max_context` | int | 10 | 1 - 100 | `--max-context` |

**默认值说明**：
- `top_p`: 1.0（OpenAI 标准默认值，不做 nucleus sampling 限制）
- `frequency_penalty`: 0.0（新增参数，默认无惩罚）
- `seed`: null（可选，不传递时输出不固定）

## 文件修改清单

### src/core/networkmanager.h
- 添加成员变量：`double m_topP`, `double m_frequencyPenalty`, `std::optional<int> m_seed`

### src/core/networkmanager.cpp
- 初始化新参数默认值
- 在 `loadSettings()` 中加载新参数
- 在 `saveSettings()` 中保存新参数
- 修复 `sendChatRequestWithContext()` 中参数硬编码问题：
  - 使用成员变量 `m_temperature` 而非硬编码 0.4/0.3
  - 将所有模型参数添加到请求 JSON
  - `seed` 仅在有值时传递
- 对 Ollama 后端改用 `/api/chat` 接口（支持 messages 格式和标准参数）

### src/cli/cli_application.cpp
- 添加 CLI 选项：`--temperature`, `--top-p`, `--max-tokens`, `--presence-penalty`, `--frequency-penalty`, `--seed`, `--max-context`
- 添加参数范围校验函数
- 在 `handleConfigCommand()` 中处理新参数并校验
- 在 `showConfig()` 中显示新参数

### cmake/config.example.json
- 更新 `maxTokens` 默认值为 8192
- 添加 `top_p`, `frequency_penalty`, `seed` 参数示例

### docs/README.md
- 更新「参数说明」表格，添加新参数

### docs/CHANGELOG.md
- 记录本次功能更新

## API 请求构建

统一使用 OpenAI 格式参数名构建请求：

```json
{
  "model": "<model_name>",
  "messages": [...],
  "temperature": 0.4,
  "top_p": 1.0,
  "max_tokens": 8192,
  "presence_penalty": 0.2,
  "frequency_penalty": 0.0,
  "stream": true,
  "seed": 42  // 仅在有值时传递
}
```

**Ollama 兼容性处理**：
- 检测 Ollama 后端（端口 11434）时，使用 `/api/chat` 接口而非 `/api/generate`
- `/api/chat` 接口支持 messages 格式和类似 OpenAI 的参数传递方式

## CLI 参数校验

在设置参数时进行范围校验，超出范围拒绝更新并提示错误：

```cpp
bool validateParameter(const QString &name, double value, double min, double max);
bool validateParameter(const QString &name, int value, int min, int max);
```

校验失败时输出：
```
错误: temperature 值 3.0 超出范围 [0.0, 2.0]
```

## CLI 使用示例

```bash
# 设置单个参数
./LocalAIAssistant-CLI config --temperature 0.7

# 设置多个参数
./LocalAIAssistant-CLI config --temperature 0.7 --top-p 0.9 --seed 42

# 查看当前配置
./LocalAIAssistant-CLI config --show-config
```

**seed 参数清除**：通过设置 `--seed -1` 或类似特殊值清除（暂不实现单独的清除命令）。

## 成功标准

1. CLI 可设置和显示所有模型参数
2. 参数值在有效范围内，超出范围拒绝并提示
3. 参数正确传递到 API 请求中
4. Ollama 和 OpenAI 后端都能正常工作
5. 文档更新完整