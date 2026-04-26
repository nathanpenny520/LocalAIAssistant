# CLI 命令行版本测试指令

CLI 可执行文件路径：
```
./build/LocalAIAssistant-CLI.app/Contents/MacOS/LocalAIAssistant-CLI
```

---

## 基础命令

| 命令 | 说明 |
|------|------|
| `chat` | 进入交互式对话模式 |
| `ask <问题>` | 单次查询 |
| `sessions` | 会话管理 |
| `config` | 配置管理 |

---

## 测试指令

### 1. 查看帮助
```bash
./LocalAIAssistant-CLI
```

### 2. 显示当前配置
```bash
./LocalAIAssistant-CLI config --show-config
```

### 3. 设置 API URL 和模型
```bash
./LocalAIAssistant-CLI config --api-url "http://127.0.0.1:11434" --model "llama3"
```

### 4. 设置外部 API 模式
```bash
./LocalAIAssistant-CLI config --external --api-url "https://api.openai.com/v1" --api-key "your-key" --model "gpt-4"
```

### 5. 设置模型参数
```bash
# 温度和 top_p
./LocalAIAssistant-CLI config --temperature 0.7 --top-p 0.9 --max-tokens 4096

# 惩罚参数
./LocalAIAssistant-CLI config --presence-penalty 0.5 --frequency-penalty 0.3

# 随机种子和上下文
./LocalAIAssistant-CLI config --seed 42 --max-context 20
```

### 6. 单次查询（流式输出）
```bash
./LocalAIAssistant-CLI ask "什么是量子计算？"
```

### 7. 单次查询（禁用流式）
```bash
./LocalAIAssistant-CLI ask "你好" --no-stream
```

### 8. 交互式对话
```bash
./LocalAIAssistant-CLI chat
```

交互模式内可用命令：
- `/help` - 帮助
- `/new` - 新会话
- `/list` - 列出会话
- `/switch <id>` - 切换会话
- `/config` - 显示配置
- `/stream` - 切换流式输出
- `/exit` - 退出

### 9. 会话管理
```bash
# 列出所有会话
./LocalAIAssistant-CLI sessions -l

# 创建新会话
./LocalAIAssistant-CLI sessions -n

# 显示会话内容
./LocalAIAssistant-CLI sessions --show <session-id>

# 删除会话
./LocalAIAssistant-CLI sessions -d <session-id>
```

---

## 参数范围验证

| 参数 | 范围 | 默认值 |
|------|------|--------|
| `--temperature` | 0.0 - 2.0 | 0.4 |
| `--top-p` | 0.0 - 1.0 | 1.0 |
| `--max-tokens` | 1 - 128000 | 8192 |
| `--presence-penalty` | -2.0 - 2.0 | 0.2 |
| `--frequency-penalty` | -2.0 - 2.0 | 0.0 |
| `--seed` | 整数, -1清除 | -1 |
| `--max-context` | 1 - 100 | 10 |

---

## 完整测试流程

```bash
# 1. 进入 CLI 目录
cd build/LocalAIAssistant-CLI.app/Contents/MacOS

# 2. 查看配置
./LocalAIAssistant-CLI config --show-config

# 3. 设置参数
./LocalAIAssistant-CLI config --temperature 0.8 --max-tokens 2048

# 4. 确认参数生效
./LocalAIAssistant-CLI config --show-config

# 5. 测试单次查询
./LocalAIAssistant-CLI ask "写一首短诗"

# 6. 进入交互模式
./LocalAIAssistant-CLI chat
```

---

## 错误测试

### 参数超出范围
```bash
# temperature 超出范围（应报错）
./LocalAIAssistant-CLI config --temperature 3.0

# max-tokens 超出范围（应报错）
./LocalAIAssistant-CLI config --max-tokens 200000
```

### 无效命令
```bash
./LocalAIAssistant-CLI unknown
```

### 缺少参数
```bash
# ask 命令缺少问题
./LocalAIAssistant-CLI ask
```