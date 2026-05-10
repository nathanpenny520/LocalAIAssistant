cmake_minimum_required(VERSION 3.16)

# Auto-detect Qt path, prioritizing official Qt over Homebrew stubs.
# Override with: cmake -B build -DQT_PATH=/custom/qt/path
if(NOT DEFINED QT_PATH)
    if(APPLE AND EXISTS "$ENV{HOME}/Qt")
        file(GLOB qt_dirs RELATIVE "$ENV{HOME}/Qt" "$ENV{HOME}/Qt/6.*")
        list(SORT qt_dirs COMPARE NATURAL)
        list(REVERSE qt_dirs)
        if(qt_dirs)
            list(GET qt_dirs 0 latest)
            set(QT_PATH "$ENV{HOME}/Qt/${latest}/macos")
        endif()
    elseif(UNIX AND NOT APPLE AND EXISTS "$ENV{HOME}/Qt")
        file(GLOB qt_dirs RELATIVE "$ENV{HOME}/Qt" "$ENV{HOME}/Qt/6.*")
        list(SORT qt_dirs COMPARE NATURAL)
        list(REVERSE qt_dirs)
        if(qt_dirs)
            list(GET qt_dirs 0 latest)
            set(QT_PATH "$ENV{HOME}/Qt/${latest}/gcc_64")
        endif()
    endif()
endif()

if(DEFINED QT_PATH AND QT_PATH)
    list(PREPEND CMAKE_PREFIX_PATH "${QT_PATH}")
endif()

project(
    LocalAIAssistant
    VERSION 1.1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(
    Qt6 REQUIRED
    COMPONENTS Widgets
               Network
               Concurrent
               Multimedia
               MultimediaWidgets
               WebSockets
               Sql
               Test)
find_package(Qt6 QUIET COMPONENTS LinguistTools)
enable_testing()
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(POPPLER poppler-cpp)
    # Phase 3: QuaZip for DOCX extraction, pugixml for XML parsing
    pkg_check_modules(PUGIXML pugixml QUIET)
endif()

# DOCX support: libzip for ZIP decompression + pugixml for XML
# Use find_library to avoid broken libzip CMake config on some platforms
find_path(libzip_INCLUDE_DIRS zip.h)
find_library(libzip_LIBRARIES NAMES zip libzip)
if(libzip_INCLUDE_DIRS AND libzip_LIBRARIES)
    set(libzip_FOUND TRUE)
    message(STATUS "libzip found, DOCX support enabled")
else()
    set(libzip_FOUND FALSE)
    message(STATUS "libzip not found, DOCX support disabled")
endif()

find_package(pugixml CONFIG QUIET)
if(pugixml_FOUND OR PUGIXML_FOUND)
    message(STATUS "pugixml found, XML parsing enabled")
else()
    message(STATUS "pugixml not found, using header-only fallback")
endif()

if(POPPLER_FOUND)
    message(STATUS "Poppler found, PDF support enabled")
else()
    message(STATUS "Poppler not found, PDF support disabled")
    add_definitions(-DNO_PDF_SUPPORT)
endif()

# ============================================================ FileParser — unified
# text/PDF/DOCX/image parsing
# ============================================================
add_library(FileParser src/parsers/fileparser.cpp src/parsers/fileparser.h)
target_include_directories(FileParser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/parsers)
target_link_libraries(FileParser PUBLIC Qt6::Core)

if(POPPLER_FOUND)
    target_include_directories(FileParser PUBLIC ${POPPLER_INCLUDE_DIRS})
    target_link_directories(FileParser PUBLIC ${POPPLER_LIBRARY_DIRS})
    target_link_libraries(FileParser PUBLIC ${POPPLER_LIBRARIES})
endif()

# DOCX support (libzip + pugixml)
if(libzip_FOUND)
    target_include_directories(FileParser PUBLIC ${libzip_INCLUDE_DIRS})
    target_link_libraries(FileParser PUBLIC ${libzip_LIBRARIES})
    target_compile_definitions(FileParser PUBLIC HAS_LIBZIP)
endif()

if(pugixml_FOUND)
    target_link_libraries(FileParser PUBLIC pugixml::pugixml)
    target_compile_definitions(FileParser PUBLIC HAS_PUGIXML)
elseif(PUGIXML_FOUND)
    target_include_directories(FileParser PUBLIC ${PUGIXML_INCLUDE_DIRS})
    target_link_directories(FileParser PUBLIC ${PUGIXML_LIBRARY_DIRS})
    target_link_libraries(FileParser PUBLIC ${PUGIXML_LIBRARIES})
    target_compile_definitions(FileParser PUBLIC HAS_PUGIXML)
endif()

if(Qt6LinguistTools_FOUND)
    set(TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/localai_zh_CN.ts
                 ${CMAKE_CURRENT_SOURCE_DIR}/translations/localai_en.ts)

    # Direct lrelease avoids qt_add_lrelease API differences across Qt 6.x versions
    set(QM_DIR ${CMAKE_CURRENT_BINARY_DIR}/translations)
    set(QM_FILES
        ${QM_DIR}/localai_zh_CN.qm
        ${QM_DIR}/localai_en.qm)

    add_custom_command(
        OUTPUT ${QM_FILES}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${QM_DIR}
        COMMAND $<TARGET_FILE:Qt6::lrelease>
                ${CMAKE_CURRENT_SOURCE_DIR}/translations/localai_zh_CN.ts
                -qm ${QM_DIR}/localai_zh_CN.qm
        COMMAND $<TARGET_FILE:Qt6::lrelease>
                ${CMAKE_CURRENT_SOURCE_DIR}/translations/localai_en.ts
                -qm ${QM_DIR}/localai_en.qm
        DEPENDS ${TS_FILES}
        COMMENT "Generating .qm translation files with lrelease")

    add_custom_target(translations_target DEPENDS ${QM_FILES})
    add_custom_target(translations ALL DEPENDS translations_target)
else()
    message(STATUS "Qt6 LinguistTools not found, translations disabled")
    add_custom_target(translations ALL)  # dummy target for add_dependencies
endif()

add_library(
    LocalAIAssistantCore
    src/core/anthropic_provider.cpp
    src/core/anthropic_provider.h
    src/core/apiprovider.cpp
    src/core/apiprovider.h
    src/core/datamodels.h
    src/core/filemanager.cpp
    src/core/filemanager.h
    src/core/llamacpp_provider.cpp
    src/core/llamacpp_provider.h
    src/core/networkmanager.cpp
    src/core/networkmanager.h
    src/core/ollama_provider.cpp
    src/core/ollama_provider.h
    src/core/openai_provider.cpp
    src/core/openai_provider.h
    src/core/sessionmanager.cpp
    src/core/sessionmanager.h
    src/prompts/promptmanager.cpp
    src/prompts/promptmanager.h
    src/ui/apptheme.cpp
    src/ui/apptheme.h)

target_include_directories(
    LocalAIAssistantCore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/prompts
           ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

target_link_libraries(LocalAIAssistantCore PUBLIC FileParser Qt6::Network Qt6::Widgets)

add_executable(
    ${PROJECT_NAME}
    src/ui/main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/markdownrenderer.cpp
    src/ui/markdownrenderer.h
    src/ui/operationconfirmdialog.cpp
    src/ui/operationconfirmdialog.h
    src/ui/settingsdialog.cpp
    src/ui/settingsdialog.h
    src/ui/stylesheetmanager.cpp
    src/ui/stylesheetmanager.h
    src/ui/translationmanager.cpp
    src/ui/translationmanager.h)

target_include_directories(
    ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ui ${CMAKE_CURRENT_SOURCE_DIR}/src/core
                            ${CMAKE_CURRENT_BINARY_DIR}/translations)

target_compile_definitions(${PROJECT_NAME}
                           PRIVATE TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}/translations")

# Windows uses relative path for translations
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TRANSLATIONS_DIR_RELATIVE="translations")
endif()

add_dependencies(${PROJECT_NAME} translations)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

# 新增 AI女友模块
add_library(
    GirlfriendModule
    src/girlfriend/avatarwidget.cpp
    src/girlfriend/avatarwidget.h
    src/girlfriend/girlfriendsession.cpp
    src/girlfriend/girlfriendsession.h
    src/girlfriend/girlfriendsessionmanager.cpp
    src/girlfriend/girlfriendsessionmanager.h
    src/girlfriend/girlfriendsettings.cpp
    src/girlfriend/girlfriendsettings.h
    src/girlfriend/girlfriendwindow.cpp
    src/girlfriend/girlfriendwindow.h
    src/girlfriend/memorymanager.cpp
    src/girlfriend/memorymanager.h
    src/girlfriend/personalityengine.cpp
    src/girlfriend/personalityengine.h
    src/girlfriend/voicemanager.cpp
    src/girlfriend/voicemanager.h)

target_include_directories(
    GirlfriendModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

target_link_libraries(
    GirlfriendModule
    PUBLIC LocalAIAssistantCore
           Qt6::Widgets
           Qt6::Multimedia
           Qt6::MultimediaWidgets
           Qt6::Network
           Qt6::WebSockets)

# 链接到主程序
target_link_libraries(${PROJECT_NAME} PRIVATE GirlfriendModule)

# ============================================================ TaskModule — 任务执行模块
# ============================================================
add_library(
    TaskModule
    src/tasks/commandexecutor.cpp
    src/tasks/commandexecutor.h
    src/tasks/operationplan.cpp
    src/tasks/operationplan.h
    src/tasks/operationundo.cpp
    src/tasks/operationundo.h
    src/tasks/safetychecker.cpp
    src/tasks/safetychecker.h
    src/tasks/taskengine.cpp
    src/tasks/taskengine.h
    src/tasks/agentloop.cpp
    src/tasks/agentloop.h)
target_include_directories(TaskModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/tasks
                                             ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(TaskModule PUBLIC LocalAIAssistantCore Qt6::Widgets)

# ============================================================ KnowledgeModule — 知识管理模块
# ============================================================
add_library(
    KnowledgeModule
    src/knowledge/docimporter.cpp
    src/knowledge/docimporter.h
    src/knowledge/embedder.cpp
    src/knowledge/embedder.h
    src/knowledge/knowledgebase.cpp
    src/knowledge/knowledgebase.h
    src/knowledge/memoryenhancer.cpp
    src/knowledge/memoryenhancer.h
    src/knowledge/textchunker.cpp
    src/knowledge/textchunker.h
    src/knowledge/vectordb.cpp
    src/knowledge/vectordb.h)
target_include_directories(KnowledgeModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/knowledge
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(
    KnowledgeModule
    PUBLIC LocalAIAssistantCore
           FileParser
           Qt6::Widgets
           Qt6::Sql
           Qt6::Concurrent)

# 如果 hnswlib 可用，启用 (header-only, 路径: third_party/hnswlib/hnswlib/hnswlib.h)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/hnswlib/hnswlib/hnswlib.h")
    target_include_directories(KnowledgeModule
                               PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hnswlib)
    target_compile_definitions(KnowledgeModule PUBLIC HNSWLIB_AVAILABLE)
    message(STATUS "hnswlib found at third_party/hnswlib/")
else()
    message(STATUS "hnswlib not found — using flat index")
    message(
        STATUS
            "  Download: git clone --depth 1 https://github.com/nmslib/hnswlib.git third_party/hnswlib-tmp"
    )
    message(
        STATUS
            "  Then: mv third_party/hnswlib-tmp/hnswlib third_party/hnswlib/ && rm -rf third_party/hnswlib-tmp"
    )
endif()

# 如果 ONNX Runtime 可用，启用
find_package(onnxruntime QUIET)
if(onnxruntime_FOUND)
    target_link_libraries(KnowledgeModule PUBLIC onnxruntime::onnxruntime)
    target_compile_definitions(KnowledgeModule PUBLIC ONNXRUNTIME_AVAILABLE)
    message(STATUS "ONNX Runtime found, real embedding enabled")
else()
    message(
        STATUS
            "ONNX Runtime not found, using placeholder embedding (install: brew install onnxruntime / apt install libonnxruntime-dev)"
    )
endif()

# 链接新模块到主程序
target_link_libraries(${PROJECT_NAME} PRIVATE TaskModule KnowledgeModule)

# CLI executable
add_executable(${PROJECT_NAME}-CLI src/cli/cli_application.cpp src/cli/cli_application.h
                                   src/cli/cli_main.cpp)

target_include_directories(
    ${PROJECT_NAME}-CLI
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core
            ${CMAKE_CURRENT_SOURCE_DIR}/src/tasks
            ${CMAKE_CURRENT_SOURCE_DIR}/src/knowledge)

target_link_libraries(${PROJECT_NAME}-CLI PRIVATE LocalAIAssistantCore TaskModule KnowledgeModule)

# readline/libedit support for better input handling (especially for Chinese characters) macOS uses
# libedit (readline-compatible), Linux uses readline
find_library(READLINE_LIBRARY readline)
find_library(EDIT_LIBRARY edit)

if(READLINE_LIBRARY)
    target_link_libraries(${PROJECT_NAME}-CLI PRIVATE ${READLINE_LIBRARY})
    target_compile_definitions(${PROJECT_NAME}-CLI PRIVATE USE_READLINE)
    message(STATUS "readline found: ${READLINE_LIBRARY}")
elseif(EDIT_LIBRARY)
    # libedit is readline-compatible (common on macOS)
    target_link_libraries(${PROJECT_NAME}-CLI PRIVATE ${EDIT_LIBRARY})
    target_compile_definitions(${PROJECT_NAME}-CLI PRIVATE USE_READLINE)
    message(STATUS "libedit found: ${EDIT_LIBRARY} (readline-compatible)")
else()
    message(STATUS "readline/libedit not found, using basic std::getline")
endif()

add_subdirectory(tests)

if(APPLE)
    # Copy icon to bundle Resources
    set(APP_ICON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns)
    set(APP_ICON_DEST ${CMAKE_CURRENT_BINARY_DIR}/app.icns)

    add_custom_command(
        OUTPUT ${APP_ICON_DEST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${APP_ICON_SOURCE} ${APP_ICON_DEST}
        DEPENDS ${APP_ICON_SOURCE}
        COMMENT "Copying app icon to build directory")

    # Copy prompts directory to build directory for development
    set(PROMPTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/prompts)
    set(PROMPTS_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/prompts)

    file(GLOB PROMPTS_DEP_FILES "${PROMPTS_SOURCE_DIR}/en/*.md" "${PROMPTS_SOURCE_DIR}/zh_CN/*.md")

    add_custom_command(
        OUTPUT ${PROMPTS_DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROMPTS_DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROMPTS_SOURCE_DIR} ${PROMPTS_DEST_DIR}
        DEPENDS ${PROMPTS_DEP_FILES}
        COMMENT "Copying prompts to build directory")

    add_custom_target(app_icon ALL DEPENDS ${APP_ICON_DEST})
    add_custom_target(copy_prompts_dev ALL DEPENDS ${PROMPTS_DEST_DIR})

    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES MACOSX_BUNDLE TRUE
                   MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in
                   RESOURCE ${APP_ICON_DEST})

    # Copy prompts to app bundle Resources after build
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/prompts
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROMPTS_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/prompts
        COMMENT "Copying prompts to app bundle Resources")

    # Copy localization directories to app bundle Resources
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/en.lproj
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/en.lproj/InfoPlist.strings
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/en.lproj/InfoPlist.strings
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/zh_CN.lproj
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/zh_CN.lproj/InfoPlist.strings
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/zh_CN.lproj/InfoPlist.strings
        COMMENT "Copying localization files to app bundle Resources")

    # Copy AIGirlfriend images to app bundle Resources
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/AIGirlfriend
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/AIGirlfriend
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/AIGirlfriend
        COMMENT "Copying AIGirlfriend images to app bundle Resources")

    # Copy app icon to app bundle Resources
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/app.icns
        COMMENT "Copying app icon to bundle Resources")

    # Copy personality.md to app bundle Resources
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/girlfriend
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/personality.md
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/girlfriend/personality.md
        COMMENT "Copying personality.md to app bundle Resources")

    # Copy memory.md to app bundle Resources
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/girlfriend
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/memory.md
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/girlfriend/memory.md
        COMMENT "Copying memory.md to app bundle Resources")

    # Copy usage docs to app bundle Resources
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/docs
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/docs/USAGE.md
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/docs/USAGE.md
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/docs/USAGE_zh_CN.md
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/docs/USAGE_zh_CN.md
        COMMENT "Copying usage docs to app bundle Resources")

    # Copy ONNX models to app bundle Resources (if exists)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/models
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-arm64.onnx
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/models/all-MiniLM-L6-v2-arm64.onnx
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-avx2.onnx
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/models/all-MiniLM-L6-v2-avx2.onnx
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer.json
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/models/tokenizer.json
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer_config.json
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/models/tokenizer_config.json
        COMMENT "Copying model files to app bundle Resources")

    # Copy .env to app bundle Resources (if exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/.env"
                    ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/.env
            COMMENT "Copying .env to app bundle Resources")
    endif()

    add_dependencies(${PROJECT_NAME} app_icon copy_prompts_dev)

    # macOS CLI .app bundle with iTerm2 detection Create CLI wrapper script that detects iTerm2
    set(CLI_WRAPPER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cli-wrapper.sh)
    set(CLI_WRAPPER_DEST ${CMAKE_CURRENT_BINARY_DIR}/cli-wrapper.sh)

    add_custom_command(
        OUTPUT ${CLI_WRAPPER_DEST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CLI_WRAPPER_SOURCE} ${CLI_WRAPPER_DEST}
        DEPENDS ${CLI_WRAPPER_SOURCE}
        COMMENT "Copying CLI wrapper script")

    # Create CLI Info.plist
    set(CLI_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/CLI-Info.plist)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CLI-Info.plist.in ${CLI_INFO_PLIST} COPYONLY)

    # Create CLI .app bundle structure
    add_custom_target(cli_bundle ALL DEPENDS ${PROJECT_NAME}-CLI ${CLI_WRAPPER_DEST})

    add_custom_command(
        TARGET cli_bundle
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant-CLI.app/Contents/MacOS
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant-CLI.app/Contents/Resources
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${CLI_WRAPPER_DEST}
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant-CLI.app/Contents/MacOS/LocalAIAssistant-CLI
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}-CLI>
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant-CLI.app/Contents/MacOS/LocalAIAssistant-CLI-bin
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CLI_INFO_PLIST}
                ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant-CLI.app/Contents/Info.plist
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant-CLI.app/Contents/Resources/app.icns
        COMMAND
            ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/models
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant-CLI.app/Contents/Resources/models
        COMMENT "Creating CLI .app bundle with iTerm2 detection")
endif()

if(WIN32)
    # Windows configuration
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    # Fix windres (MinGW) path space issues by using temp file instead of popen
    if(MINGW)
        set(CMAKE_RC_FLAGS "--use-temp-file")
    endif()

    # Windows icon resource (requires app.ico) Copy ico to build directory first to avoid path space
    # issues
    set(APP_ICON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
    set(APP_ICON_DEST "${CMAKE_CURRENT_BINARY_DIR}/app.ico")

    configure_file(${APP_ICON_SOURCE} ${APP_ICON_DEST} COPYONLY)

    # Copy rc to build directory (rc uses relative path "app.ico")
    set(APP_RC_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.rc")
    set(APP_RC_DEST "${CMAKE_CURRENT_BINARY_DIR}/app.rc")

    configure_file(${APP_RC_SOURCE} ${APP_RC_DEST} COPYONLY)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
        target_sources(${PROJECT_NAME} PRIVATE ${APP_RC_DEST})

        # Copy icon to executable directory for runtime loading
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${APP_ICON_DEST}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/app.ico"
            COMMENT "Copying app.ico to executable directory")

        # CLI executable icon (uses same app.ico)
        target_sources(${PROJECT_NAME}-CLI PRIVATE ${APP_RC_DEST})

        # Copy icon to CLI executable directory for runtime loading
        add_custom_command(
            TARGET ${PROJECT_NAME}-CLI
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${APP_ICON_DEST}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}-CLI>/app.ico"
            COMMENT "Copying app.ico to CLI executable directory")
    else()
        message(WARNING "Windows icon file app.ico not found. Please create it from app.icns.")
    endif()

    # Copy prompts to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/prompts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/src/prompts"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/prompts"
        COMMENT "Copying prompts to build directory")

    # Also copy soul.md for backwards compatibility
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core/soul.md"
        COMMENT "Copying soul.md to build directory (legacy)")

    # Copy AIGirlfriend images to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/AIGirlfriend"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/AIGirlfriend"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/AIGirlfriend"
        COMMENT "Copying AIGirlfriend images to build directory")

    # Copy personality.md to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/girlfriend"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/personality.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/girlfriend/personality.md"
        COMMENT "Copying personality.md to build directory")

    # Copy memory.md to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/memory.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/girlfriend/memory.md"
        COMMENT "Copying memory.md to build directory")

    # Copy .env to build directory (if exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/.env"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.env"
            COMMENT "Copying .env to build directory")
    endif()

    # Copy translations to build directory (if LinguistTools available)
    if(Qt6LinguistTools_FOUND)
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations"
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/localai_zh_CN.qm"
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/localai_en.qm"
            COMMENT "Copying translations to build directory")
    endif()

    # Copy usage docs to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/docs"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/docs/USAGE.md"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/docs/USAGE.md"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/docs/USAGE_zh_CN.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/docs/USAGE_zh_CN.md"
        COMMENT "Copying usage docs to build directory")

    # Copy ONNX models to build directory (if exists)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-arm64.onnx"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/all-MiniLM-L6-v2-arm64.onnx"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-avx2.onnx"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/all-MiniLM-L6-v2-avx2.onnx"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/tokenizer.json"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer_config.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/tokenizer_config.json"
        COMMENT "Copying model files to build directory")
endif()

if(UNIX AND NOT APPLE)
    # Linux configuration Copy prompts to build directory for development
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/prompts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/src/prompts"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/prompts"
        COMMENT "Copying prompts to build directory")

    # Also copy soul.md for backwards compatibility
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core/soul.md"
        COMMENT "Copying soul.md to build directory (legacy)")

    # Copy AIGirlfriend images to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/AIGirlfriend"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/AIGirlfriend"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/AIGirlfriend"
        COMMENT "Copying AIGirlfriend images to build directory")

    # Copy personality.md to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/girlfriend"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/personality.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/girlfriend/personality.md"
        COMMENT "Copying personality.md to build directory")

    # Copy memory.md to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/memory.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/girlfriend/memory.md"
        COMMENT "Copying memory.md to build directory")

    # Copy .env to build directory (if exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/.env"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.env"
            COMMENT "Copying .env to build directory")
    endif()

    # Copy translations to build directory (if LinguistTools available)
    if(Qt6LinguistTools_FOUND)
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations"
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/localai_zh_CN.qm"
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/localai_en.qm"
            COMMENT "Copying translations to build directory")
    endif()

    # Copy ONNX models to build directory (if exists)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-arm64.onnx"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/all-MiniLM-L6-v2-arm64.onnx"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-avx2.onnx"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/all-MiniLM-L6-v2-avx2.onnx"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/tokenizer.json"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer_config.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models/tokenizer_config.json"
        COMMENT "Copying model files to build directory")

    # Copy usage docs to build directory
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/docs"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/docs/USAGE.md"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/docs/USAGE.md"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/docs/USAGE_zh_CN.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/docs/USAGE_zh_CN.md"
        COMMENT "Copying usage docs to build directory")
endif()

install(TARGETS ${PROJECT_NAME}-CLI RUNTIME DESTINATION bin)

if(APPLE)
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION /Applications)
    if(Qt6LinguistTools_FOUND)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm
                      ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm
                DESTINATION LocalAIAssistant.app/Contents/Resources/translations)
    endif()
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns
        DESTINATION LocalAIAssistant.app/Contents/Resources
        RENAME app.icns)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md
            DESTINATION LocalAIAssistant.app/Contents/Resources/core)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/prompts/
            DESTINATION LocalAIAssistant.app/Contents/Resources/prompts)
endif()

if(WIN32)
    install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-CLI RUNTIME DESTINATION bin)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md DESTINATION bin/core)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/prompts/ DESTINATION bin/prompts)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AIGirlfriend/ DESTINATION bin/AIGirlfriend)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/personality.md
                  ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/memory.md DESTINATION bin/girlfriend)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/ DESTINATION bin/docs)
    if(Qt6LinguistTools_FOUND)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm
                      ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm
                DESTINATION bin/translations)
    endif()
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md
            DESTINATION share/localaiassistant/core)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/prompts/
            DESTINATION share/localaiassistant/prompts)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AIGirlfriend/
            DESTINATION share/localaiassistant/AIGirlfriend)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/personality.md
                  ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/memory.md
            DESTINATION share/localaiassistant/girlfriend)
    if(Qt6LinguistTools_FOUND)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm
                      ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm
                DESTINATION share/localaiassistant/translations)
    endif()
    # Install ONNX models (if exist)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-arm64.onnx")
        install(
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-arm64.onnx
                  ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/all-MiniLM-L6-v2-avx2.onnx
                  ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer.json
                  ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/tokenizer_config.json
            DESTINATION share/localaiassistant/models)
    endif()
    # Install desktop entry and icon (if they exist)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/localaiassistant.desktop")
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/localaiassistant.desktop
                DESTINATION share/applications)
    endif()
endif()
