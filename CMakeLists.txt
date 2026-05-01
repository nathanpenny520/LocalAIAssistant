cmake_minimum_required(VERSION 3.16)
project(LocalAIAssistant VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools Multimedia)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(POPPLER poppler-cpp)
endif()

if(POPPLER_FOUND)
    message(STATUS "Poppler found, PDF support enabled")
else()
    message(STATUS "Poppler not found, PDF support disabled")
    add_definitions(-DNO_PDF_SUPPORT)
endif()

set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/localai_zh_CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/localai_en.ts
)

qt_add_lrelease(TS_FILES ${TS_FILES} LRELEASE_TARGET translations_target)
add_custom_target(translations ALL DEPENDS ${translations_target})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/translations)

add_custom_command(TARGET translations_target POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/localai_zh_CN.qm
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/localai_en.qm
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm
)

add_library(LocalAIAssistantCore
    src/core/networkmanager.cpp
    src/core/networkmanager.h
    src/core/sessionmanager.cpp
    src/core/sessionmanager.h
    src/core/datamodels.h
    src/core/filemanager.cpp
    src/core/filemanager.h
)

target_include_directories(LocalAIAssistantCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

if(POPPLER_FOUND)
    target_include_directories(LocalAIAssistantCore PUBLIC
        ${POPPLER_INCLUDE_DIRS}
    )
    target_link_directories(LocalAIAssistantCore PUBLIC
        ${POPPLER_LIBRARY_DIRS}
    )
endif()

target_link_libraries(LocalAIAssistantCore PUBLIC
    Qt6::Network
)

if(POPPLER_FOUND)
    target_link_libraries(LocalAIAssistantCore PUBLIC
        ${POPPLER_LIBRARIES}
    )
endif()

add_executable(${PROJECT_NAME}
    src/ui/main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/settingsdialog.cpp
    src/ui/settingsdialog.h
    src/ui/stylesheetmanager.cpp
    src/ui/stylesheetmanager.h
    src/ui/translationmanager.cpp
    src/ui/translationmanager.h
    src/ui/markdownrenderer.cpp
    src/ui/markdownrenderer.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_BINARY_DIR}/translations
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}/translations"
)

add_dependencies(${PROJECT_NAME} translations)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LocalAIAssistantCore
    Qt6::Widgets
)

# 新增 AI女友模块
add_library(GirlfriendModule
    src/girlfriend/girlfriendwindow.cpp
    src/girlfriend/girlfriendwindow.h
    src/girlfriend/avatarwidget.cpp
    src/girlfriend/avatarwidget.h
    src/girlfriend/personalityengine.cpp
    src/girlfriend/personalityengine.h
    src/girlfriend/girlfriendsession.cpp
    src/girlfriend/girlfriendsession.h
)

target_include_directories(GirlfriendModule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(GirlfriendModule PUBLIC
    LocalAIAssistantCore
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)

# 链接到主程序
target_link_libraries(${PROJECT_NAME} PRIVATE
    GirlfriendModule
)

add_executable(${PROJECT_NAME}-CLI
    src/cli/cli_main.cpp
    src/cli/cli_application.cpp
    src/cli/cli_application.h
)

target_include_directories(${PROJECT_NAME}-CLI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(${PROJECT_NAME}-CLI PRIVATE
    LocalAIAssistantCore
)

if(APPLE)
    # Copy icon to bundle Resources
    set(APP_ICON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns)
    set(APP_ICON_DEST ${CMAKE_CURRENT_BINARY_DIR}/app.icns)

    add_custom_command(
        OUTPUT ${APP_ICON_DEST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${APP_ICON_SOURCE} ${APP_ICON_DEST}
        DEPENDS ${APP_ICON_SOURCE}
        COMMENT "Copying app icon to build directory"
    )

    # Copy soul.md to build directory for development
    set(SOUL_MD_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md)
    set(SOUL_MD_DEST ${CMAKE_CURRENT_BINARY_DIR}/core/soul.md)

    add_custom_command(
        OUTPUT ${SOUL_MD_DEST}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/core
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOUL_MD_SOURCE} ${SOUL_MD_DEST}
        DEPENDS ${SOUL_MD_SOURCE}
        COMMENT "Copying soul.md to build directory"
    )

    add_custom_target(app_icon ALL DEPENDS ${APP_ICON_DEST})
    add_custom_target(soul_md ALL DEPENDS ${SOUL_MD_DEST})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in
        RESOURCE ${APP_ICON_DEST}
    )

    # Copy soul.md to app bundle Resources after build
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/core
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOUL_MD_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/core/soul.md
        COMMENT "Copying soul.md to app bundle Resources"
    )

    # Copy localization directories to app bundle Resources
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/en.lproj
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/en.lproj/InfoPlist.strings
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/en.lproj/InfoPlist.strings
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/zh_CN.lproj
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/zh_CN.lproj/InfoPlist.strings
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/zh_CN.lproj/InfoPlist.strings
        COMMENT "Copying localization files to app bundle Resources"
    )

    # Copy AIGirlfriend images to app bundle Resources
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/AIGirlfriend
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/AIGirlfriend
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/AIGirlfriend
        COMMENT "Copying AIGirlfriend images to app bundle Resources"
    )

    # Copy personality.md to app bundle Resources
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/girlfriend
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/src/girlfriend/personality.md
            ${CMAKE_CURRENT_BINARY_DIR}/LocalAIAssistant.app/Contents/Resources/girlfriend/personality.md
        COMMENT "Copying personality.md to app bundle Resources"
    )

    add_dependencies(${PROJECT_NAME} app_icon soul_md)
endif()

if(WIN32)
    # Windows configuration
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Copy soul.md to build directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core/soul.md"
        COMMENT "Copying soul.md to build directory"
    )

    # Copy translations to app bundle Resources directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/translations/localai_zh_CN.qm"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/translations/localai_en.qm"
        COMMENT "Copying translations to app bundle Resources"
    )
endif()

if(UNIX AND NOT APPLE)
    # Linux configuration
    # Copy soul.md to build directory for development
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core/soul.md"
        COMMENT "Copying soul.md to build directory"
    )
endif()

install(TARGETS ${PROJECT_NAME}-CLI
    RUNTIME DESTINATION bin
)

if(APPLE)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION /Applications
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm
        DESTINATION LocalAIAssistant.app/Contents/Resources/translations
    )
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns
        DESTINATION LocalAIAssistant.app/Contents/Resources
        RENAME app.icns
    )
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md
        DESTINATION LocalAIAssistant.app/Contents/Resources/core
    )
endif()

if(WIN32)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md
        DESTINATION bin/core
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm
        DESTINATION bin/translations
    )
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/soul.md
        DESTINATION share/localaiassistant/core
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_zh_CN.qm
        ${CMAKE_CURRENT_BINARY_DIR}/translations/localai_en.qm
        DESTINATION share/localaiassistant/translations
    )
    # Install desktop entry and icon (if they exist)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/localaiassistant.desktop")
        install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/localaiassistant.desktop
            DESTINATION share/applications
        )
    endif()
endif()
