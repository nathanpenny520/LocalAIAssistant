#!/bin/bash
set -euo pipefail

# ============================================================
# LocalAIAssistant - Code Formatter
# ============================================================
# Runs all project formatters: clang-format, cmake-format,
# shfmt, and prettier.
# Usage: ./scripts/format.sh [--check]
#
# Without flags, formats in-place.
# With --check, exits non-zero if any file would change.
# ============================================================

CHECK_MODE=false
case "${1:-}" in
    --check)
        CHECK_MODE=true
        ;;
    -h|--help)
        echo "Usage: ./scripts/format.sh [OPTIONS]"
        echo ""
        echo "Run all project formatters: clang-format, cmake-format, shfmt, prettier."
        echo ""
        echo "Options:"
        echo "  --check       Dry-run: exit non-zero if any file would change"
        echo "  -h, --help    Show this help message and exit"
        echo ""
        echo "Without flags, formats all files in-place."
        exit 0
        ;;
esac

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

# --- C++ (clang-format) ---
echo "==> C++ (clang-format)"
if $CHECK_MODE; then
    find src/ tests/ -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
        | xargs -0 clang-format --dry-run -Werror
else
    find src/ tests/ -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
        | xargs -0 clang-format -i
fi

# --- CMake (cmake-format) ---
echo "==> CMake (cmake-format)"
if $CHECK_MODE; then
    find . -name 'CMakeLists.txt' -not -path './build/*' -not -path './.git/*' \
        -print0 | xargs -0 cmake-format --check
else
    find . -name 'CMakeLists.txt' -not -path './build/*' -not -path './.git/*' \
        -print0 | xargs -0 cmake-format -i
fi

# --- Shell scripts (shfmt) ---
echo "==> Shell (shfmt)"
if $CHECK_MODE; then
    shfmt -i 4 -ci -bn -d scripts/*.sh
else
    shfmt -i 4 -ci -bn -w scripts/*.sh
fi

echo "==> Formatting complete."
