# ============================================================
# Test targets for LocalAIAssistant
# ============================================================

# Shared test utility — brings in common Qt Test boilerplate
macro(add_unit_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/tasks
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/knowledge
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/prompts
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/girlfriend
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/parsers
    )
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        ${ARGN}
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

add_unit_test(test_safetychecker test_safetychecker.cpp TaskModule)
add_unit_test(test_agentloop test_agentloop.cpp TaskModule Qt6::Widgets)
add_unit_test(test_commandexecutor test_commandexecutor.cpp TaskModule Qt6::Widgets)
add_unit_test(test_sessionmanager test_sessionmanager.cpp LocalAIAssistantCore Qt6::Network)
add_unit_test(test_fileparser test_fileparser.cpp LocalAIAssistantCore Qt6::Network)

# ── UI/Theme tests ──────────────────────────────────────────
# UI sources are not in a shared library, so compile them directly.

add_executable(test_apptheme
    test_apptheme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/apptheme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/stylesheetmanager.cpp
)
target_include_directories(test_apptheme PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
)
target_link_libraries(test_apptheme PRIVATE Qt6::Test Qt6::Widgets)
add_test(NAME test_apptheme COMMAND test_apptheme)

add_executable(test_stylesheetmanager
    test_stylesheetmanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/apptheme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/stylesheetmanager.cpp
)
target_include_directories(test_stylesheetmanager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
)
target_link_libraries(test_stylesheetmanager PRIVATE Qt6::Test Qt6::Widgets)
add_test(NAME test_stylesheetmanager COMMAND test_stylesheetmanager)

add_executable(test_markdownrenderer
    test_markdownrenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/apptheme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/stylesheetmanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/markdownrenderer.cpp
)
target_include_directories(test_markdownrenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
)
target_link_libraries(test_markdownrenderer PRIVATE Qt6::Test Qt6::Widgets)
add_test(NAME test_markdownrenderer COMMAND test_markdownrenderer)
