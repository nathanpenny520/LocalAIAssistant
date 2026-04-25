cmake_minimum_required(VERSION 3.16)
project(LocalAIAssistant VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools)

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
)

target_include_directories(LocalAIAssistantCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(LocalAIAssistantCore PUBLIC
    Qt6::Network
)

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

    add_custom_target(app_icon ALL DEPENDS ${APP_ICON_DEST})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in
        RESOURCE ${APP_ICON_DEST}
    )

    add_dependencies(${PROJECT_NAME} app_icon)
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
endif()
