# Contributing to LocalAIAssistant

## Development Environment

### Prerequisites

- **Qt 6.5+** with Network and Widgets modules
- **CMake 3.16+**
- **C++17** compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Optional: Poppler (PDF), libzip + pugixml (DOCX), ONNX Runtime (embeddings), hnswlib (vector DB), readline (CLI)

### Setup

```bash
git clone <repo-url>
cd sourcecode-ai-assistant
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
```

If Qt is not in your system PATH, specify it:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQT_PATH=/path/to/Qt/6.x.x/macos
```

## Commit Convention

Format: `type(scope): brief description`

Types: `feat`, `fix`, `refactor`, `docs`, `test`, `chore`

Examples:
- `feat(core): add Anthropic provider`
- `fix(ui): resolve markdown code block rendering`
- `test(session): add removeSession regression test`

## Pull Request Process

1. Create a feature branch from `main`
2. Make changes and commit following the convention above
3. Run `cmake --build build --parallel 4` and ensure it succeeds
4. Run `ctest` and ensure all tests pass
5. Open a PR with the provided template
6. Ensure all checks pass before merging

## Code Standards

See [CLAUDE.md](CLAUDE.md) for full code standards including:
- File and function length limits (500 / 50 lines)
- Naming conventions (snake_case files, PascalCase types, m_ members)
- Include order and formatting rules

## Testing

- Framework: Qt Test
- Run: `ctest` from the `build/` directory
- All new code requires tests
- Bug fixes require regression tests

## Formatters

```bash
./scripts/format.sh           # Format all code
./scripts/format.sh --check   # Dry-run check
```

Tools: clang-format, cmake-format, shfmt, prettier
