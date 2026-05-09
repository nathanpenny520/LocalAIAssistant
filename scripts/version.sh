#!/bin/bash

# ============================================================
# LocalAIAssistant - Shared Version Utility
# ============================================================
# Sourced by build.sh and package.sh to ensure a single
# source of truth for version extraction.
#
# Usage: source "$(dirname "$0")/version.sh"
#        version=$(get_version)
# ============================================================

get_version() {
    grep -oE 'VERSION [0-9]+\.[0-9]+\.[0-9]+' "$PROJECT_ROOT/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1
}
