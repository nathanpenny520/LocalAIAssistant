# Build & Utility Scripts

## Overview

| Script | Purpose |
|--------|---------|
| `build.sh` | Cross-platform build, test, and packaging |
| `package.sh` | Standalone artifact packaging (DMG/ZIP/tar.gz) |
| `setup.sh` | First-clone dependency detection |
| `format.sh` | Code formatter runner (check or apply) |
| `version.sh` | Shared version number extraction (sourced, not run directly) |
| `cli-wrapper.sh` | macOS CLI .app bundle launcher with iTerm2 detection |

---

## build.sh

The main entry point for building, testing, running, and packaging.

```bash
./build.sh                        # Release build all targets, then prompt to run
./build.sh -c                     # Clean rebuild
./build.sh -c -d --no-run         # Clean debug build, don't prompt for run
./build.sh run --gui              # Build and launch GUI
./build.sh run --cli              # Build and launch CLI
./build.sh build -p               # Build + package for distribution
./build.sh package                # Package existing build artifacts only
./build.sh test                   # Build and run unit tests
./build.sh LocalAIAssistant-CLI   # Build CLI target only
./build.sh LocalAIAssistant       # Build GUI target only
./build.sh -q /path/to/qt         # Override Qt path
./build.sh help                   # Show full help
```

| Flag | Description |
|------|-------------|
| `-c` | Clean build (remove `build/` first) |
| `-d` | Debug build (`CMAKE_BUILD_TYPE=Debug`) |
| `-r` | Release build (default; `CMAKE_BUILD_TYPE=Release`) |
| `-v` | Verbose output |
| `-q <path>` | Override Qt installation path |
| `-j <N>` | Override parallel job count |
| `-p` | Package after build (shorthand for `build -p`) |
| `-h` | Show help |
| `--no-run` | Skip the post-build run prompt |
| `--gui` | Run GUI after build |
| `--cli` | Run CLI after build |
| `--help-only` | Show help and exit without building |

**Commands:** `build`, `run`, `test`, `package`, `help`, or a CMake target name.

The script auto-detects Qt and compiler on each platform (macOS Homebrew / Windows Qt default paths / Linux pkg-config).

---

## package.sh

Creates distributable artifacts. Typically called via `build.sh build -p`, but can be run standalone against an existing build.

```bash
./package.sh                           # Package current build
./package.sh --build-dir /path/to/build  # Package specific build dir
```

| Platform | Output |
|----------|--------|
| macOS | `release/LocalAIAssistant-x.x.x-macOS.dmg` |
| Windows | `release/LocalAIAssistant-x.x.x-Windows-x64.zip` |
| Linux | `release/LocalAIAssistant-x.x.x-Linux-x86_64.tar.gz` |

All artifacts include a SHA256SUM file. macOS uses `macdeployqt` + `hdiutil`. Windows uses `windeployqt`. Linux bundles an `install.sh`.

---

## setup.sh

Run once after cloning to check that all build dependencies are available and create the `.env` configuration file from `.env.example`.

```bash
./scripts/setup.sh
```

Detects: CMake, C++ compiler, Qt 6 (base + Multimedia + WebSockets), Poppler, Readline (or libedit), libzip, pugixml, ONNX Runtime. Reports missing dependencies with platform-specific install instructions. Prompts for iFLYTEK credential configuration during `.env` creation.

---

## format.sh

Runs code formatters across the project.

```bash
./scripts/format.sh               # Apply formatting (modifies files)
./scripts/format.sh --check       # Dry-run: report issues without modifying
```

Tools used: `clang-format` (C++), `cmake-format` (CMake), `shfmt` (shell scripts).

---

## version.sh

Extracts the project version from `CMakeLists.txt`. Designed to be **sourced** by other scripts, not run directly.

```bash
source scripts/version.sh         # Defines get_version() function
version=$(get_version)            # Call with $PROJECT_ROOT set
```

`PROJECT_ROOT` must be set before calling `get_version()` (typically `SCRIPT_DIR/..`).

---

## cli-wrapper.sh

macOS-only. Used internally by the CLI `.app` bundle's `Info.plist` to launch the CLI binary inside the user's preferred terminal (iTerm2 if installed, otherwise Terminal.app).
