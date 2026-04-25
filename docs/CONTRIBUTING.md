# 贡献指南

感谢您考虑为本项目做出贡献！

---

## 如何贡献

### 报告问题

如果您发现了 bug 或有功能建议：

1. 在 GitHub Issues 中搜索，确认问题未被报告
2. 创建新 Issue，包含：
   - 清晰的问题标题
   - 详细的问题描述
   - 重现步骤（如果是 bug）
   - 预期行为与实际行为
   - 系统环境信息（OS、Qt 版本等）

### 提交代码

1. **Fork 项目**
   ```bash
   git clone https://github.com/YOUR_USERNAME/sourcecode-ai-assistant.git
   ```

2. **创建分支**
   ```bash
   git checkout -b feature/your-feature-name
   # 或
   git checkout -b fix/your-fix-name
   ```

3. **编写代码**
   - 遵循项目代码风格
   - 添加必要的注释
   - 确保代码可编译

4. **测试**
   ```bash
   ./scripts/build.sh -c    # 清理构建
   ./scripts/demo.sh        # 运行演示
   ```

5. **提交变更**
   ```bash
   git add .
   git commit -m "描述性的提交信息"
   ```

6. **推送并创建 PR**
   ```bash
   git push origin feature/your-feature-name
   ```
   然后在 GitHub 上创建 Pull Request

---

## 代码规范

### C++ 代码风格

- 使用 C++17 特性
- 遵循 Qt 编码规范
- 类名使用 PascalCase：`NetworkManager`
- 方法名使用 camelCase：`sendChatRequest`
- 成员变量使用 `m_` 前缀：`m_networkManager`
- 常量使用全大写：`MAX_TOKENS`

### 注释规范

```cpp
// 单行注释用于简短说明

/**
 * 多行注释用于详细说明
 * @param message 用户消息
 * @return 响应内容
 */
QString processMessage(const QString &message);
```

### Git 提交信息

```
类型: 简短描述

详细说明（可选）

- 类型: feat（新功能）/ fix（修复）/ docs（文档）/ 
        refactor（重构）/ test（测试）/ chore（其他）
- 示例:
  feat: 添加导出会话功能
  fix: 修复流式输出中断问题
  docs: 更新构建文档
```

---

## 项目结构

```
src/
├── core/           # 核心业务逻辑（NetworkManager, SessionManager）
├── ui/             # GUI 界面（MainWindow, SettingsDialog）
└── cli/            # CLI 命令行（CLIApplication）
```

修改时请遵循分层架构原则：
- **表示层** (ui/, cli/) 仅处理用户交互
- **业务逻辑层** (core/) 处理网络请求、会话管理
- **数据层** 处理存储和配置

---

## 开发环境设置

```bash
# macOS
brew install cmake qt@6

# Ubuntu/Debian
sudo apt install cmake qt6-base-dev qt6-base-dev-tools

# Windows
# 安装 Visual Studio + Qt 官网下载
```

构建：
```bash
./scripts/build.sh -d    # Debug 模式，便于调试
```

---

## 行为准则

- 尊重所有贡献者
- 保持专业、友善的交流
- 接受建设性批评
- 关注项目本身，而非个人

---

## 联系方式

如有问题，请：
- 提交 Issue
- 或在 Pull Request 中讨论

---

**感谢您的贡献！**