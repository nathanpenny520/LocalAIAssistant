# Phase 4 Layer 3: MCP Integration Plan

## Context

Add Model Context Protocol (MCP) support so users can configure external tool servers (Brave Search, filesystem, etc.) that the AI can call during agent-loop task execution. MCP servers run as separate processes communicating via stdin/stdout JSON-RPC 2.0. This is the "Layer 3" from ROADMAP Phase 4 — it does NOT require Layers 1/2 (native plugins or JavaScript plugins) to be built first.

## Architecture Overview

MCP adds a new operation type (`McpToolCall`) that plugs into the existing task execution pipeline without changing its structure:

```
AI outputs [TASK_PLAN] JSON (now includes "type": "mcp_tool")
  → TaskEngine::parsePlanFromJson() parses McpToolCall operations
    → SafetyChecker treats McpToolCall as Safe (no filesystem/shell access)
      → CommandExecutor::executeMcpTool() dispatches to McpClientManager
        → McpClientManager::callToolBlocking() sends JSON-RPC via QProcess stdio
          → Result fed back via AgentLoop::buildResultFeedback() as [ITERATION_FEEDBACK]
```

Key design decisions:
- **Synchronous blocking via QEventLoop** for tool calls — follows existing `CommandExecutor::runCommand()` pattern exactly
- **McpClientManager is a singleton** — follows `TaskEngine`, `AgentLoop` pattern
- **McpClient is NOT a singleton** — one per configured server, owned by the manager
- **Zero new dependencies** — QProcess, QJsonDocument, QTimer, QEventLoop are all available via Qt6::Core

## New Files

All in `src/core/` (part of `LocalAIAssistantCore` library):

| File | Lines | Purpose |
|---|---|---|
| `mcp_types.h` | ~80 | Header-only value types: `McpServerConfig`, `McpTool`, `McpJsonRpcRequest`, `McpJsonRpcResponse`, `McpJsonRpcError` |
| `mcp_jsonrpc.h` / `.cpp` | ~40 / 120 | Serialize/parse JSON-RPC 2.0 messages, newline framing, ID generation, standard method builders |
| `mcp_client.h` / `.cpp` | ~100 / 350 | Persistent async QProcess for one MCP server: launch, initialize handshake, tool discovery, tool call, auto-restart with backoff, timeout |
| `mcp_client_manager.h` / `.cpp` | ~80 / 250 | Multi-server singleton: load `mcp_servers.json`, manage `McpClient` lifecycle, aggregate tools, blocking `callToolBlocking()`, prompt text generation |

Test files in `tests/`:

| File | Lines |
|---|---|
| `test_mcp_jsonrpc.cpp` | ~150 |
| `test_mcp_client_manager.cpp` | ~120 |

## Modified Files

| File | Change |
|---|---|
| [src/tasks/operationplan.h](../src/tasks/operationplan.h) | Add `McpToolCall` to `ShellOperation::Type` enum; add `QJsonObject mcpArguments` field |
| [src/tasks/taskengine.cpp](../src/tasks/taskengine.cpp) | Add `"mcp_tool"`/`"mcptool"` parsing in `parsePlanFromJson()`; extract `arguments` field |
| [src/tasks/commandexecutor.h](../src/tasks/commandexecutor.h) | Add `executeMcpTool()` private method declaration |
| [src/tasks/commandexecutor.cpp](../src/tasks/commandexecutor.cpp) | Add `executeMcpTool()` implementation + dispatch case in `execute()` |
| [src/tasks/safetychecker.cpp](../src/tasks/safetychecker.cpp) | Add `McpToolCall` → `Safe` in `dangerLevel()`; add `McpToolCall` → `Approved` in `validateOperation()` |
| [CMakeLists.txt](../CMakeLists.txt) | Add 6 new source files to `LocalAIAssistantCore` library |
| [tests/CMakeLists.txt](../tests/CMakeLists.txt) | Add 2 new test targets |
| [src/prompts/en/task.md](../src/prompts/en/task.md) | Add `{{mcp_tools}}` placeholder |
| [src/prompts/zh_CN/task.md](../src/prompts/zh_CN/task.md) | Add `{{mcp_tools}}` placeholder |

## mcp_servers.json Format

```json
{
  "mcpServers": {
    "brave-search": {
      "command": "npx",
      "args": ["-y", "@anthropic/mcp-server-brave-search"],
      "env": {"BRAVE_API_KEY": "your-key"}
    }
  }
}
```

Search paths (following `EnvConfig::findEnvFile` pattern):
1. `<appDir>/mcp_servers.json`
2. `<appDir>/../Resources/mcp_servers.json` (macOS bundle)
3. `./mcp_servers.json` (working directory — development)

Config is lazy-loaded at startup. If the file doesn't exist, MCP is simply disabled (no error).

## MCP JSON-RPC 2.0 Protocol (stdio transport)

- Messages are newline-delimited JSON (one complete JSON object + `\n` per line)
- Client sends: `initialize` → receives capabilities → sends `initialized` notification
- Tool discovery: client sends `tools/list`, server responds with tool name/description/inputSchema
- Tool invocation: client sends `tools/call` with tool name + arguments, server responds with result

### Message Format
```json
// Request:
{"jsonrpc": "2.0", "id": 1, "method": "tools/list", "params": {}}

// Response:
{"jsonrpc": "2.0", "id": 1, "result": {"tools": [...]}}

// Error:
{"jsonrpc": "2.0", "id": 1, "error": {"code": -32600, "message": "..."}}

// Notification (no id):
{"jsonrpc": "2.0", "method": "notifications/initialized"}
```

## Integration Points in Detail

### 1. ShellOperation changes (`operationplan.h:20-29`)

Add to enum:
```cpp
McpToolCall  // invoke an MCP tool — no filesystem or shell access
```
Add to struct:
```cpp
QJsonObject mcpArguments;  // MCP tool call arguments (only for McpToolCall)
```

### 2. TaskEngine parsing (`taskengine.cpp:66-101`)

After existing type dispatch, add:
```cpp
else if (typeStr == QStringLiteral("mcp_tool") || typeStr == QStringLiteral("mcptool"))
    op.type = ShellOperation::McpToolCall;
```
After field extraction (line 109), add:
```cpp
if (op.type == ShellOperation::McpToolCall)
    op.mcpArguments = opObj[QStringLiteral("arguments")].toObject();
```

AI uses format:
```json
{"type": "mcp_tool", "command": "brave_web_search", "arguments": {"query": "..."}, "description": "Search web"}
```

### 3. CommandExecutor dispatch (`commandexecutor.cpp:394`)

Add to `execute()` switch:
```cpp
case ShellOperation::McpToolCall:
    return executeMcpTool(op);
```

`executeMcpTool()` calls `McpClientManager::instance()->callToolBlocking(toolName, op.mcpArguments, op.timeoutSecs)` using the same QEventLoop blocking pattern as the existing `runCommand()`.

### 4. SafetyChecker (`safetychecker.cpp:45-54, 155-170`)

In `dangerLevel()`: add `McpToolCall` to the `Safe` return group.
In `validateOperation()`: `McpToolCall` returns `Approved` immediately (no path access, no shell). Blocked only if tool name is empty.

### 5. Prompt injection

Add `{{mcp_tools}}` template variable to `task.md` files. `McpClientManager::toolNamesForPrompt()` generates a markdown list of available tools with descriptions and argument schemas. `PromptManager` substitutes the variable at load time (following the existing `{{current_datetime}}` pattern).

Generated prompt content example:
```markdown
## Available MCP Tools

You can call external tools using the `mcp_tool` operation type. Available tools:

**brave_web_search**
- Description: Performs a web search using Brave Search API
- Arguments: `{"query": "search query string", "count": number}`
- Example: `{"type": "mcp_tool", "command": "brave_web_search", "arguments": {"query": "C++17 features"}, "description": "Search for C++17"}`

**brave_local_search**
- Description: Searches for local businesses and places
- Arguments: `{"query": "search query string", "count": number}`
- Example: `{"type": "mcp_tool", "command": "brave_local_search", "arguments": {"query": "coffee shops"}, "description": "Find nearby coffee shops"}`
```

## Implementation Order

| Step | Files | Test |
|---|---|---|
| 1. mcp_types.h + mcp_jsonrpc | 3 new files | `test_mcp_jsonrpc` — 10 unit tests |
| 2. mcp_client | 2 new files | Manual test with a real MCP server |
| 3. mcp_client_manager | 2 new files | `test_mcp_client_manager` — 6 unit tests |
| 4. TaskEngine + CommandExecutor + SafetyChecker integration | 5 modified existing files | Build + manual e2e test with Brave Search |
| 5. Prompt injection + CMake | task.md × 2, CMakeLists.txt, tests/CMakeLists.txt | Full build + ctest |

## Verification

1. `cmake --build build --parallel 4` succeeds
2. `ctest` — all 10 test suites pass (8 existing + 2 new)
3. Create `mcp_servers.json` with a test MCP server → launch GUI → verify tools appear in system prompt
4. Send a task that requires web search → verify AI generates `"type": "mcp_tool"` → tool call executes → result feeds back
5. Server crash → auto-restart triggers → tools rediscovered
6. Missing/malformed `mcp_servers.json` → app launches normally, MCP disabled
