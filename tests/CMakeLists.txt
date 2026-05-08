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
    )
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        ${ARGN}
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

add_unit_test(test_safetychecker test_safetychecker.cpp TaskModule)
add_unit_test(test_commandexecutor test_commandexecutor.cpp TaskModule Qt6::Widgets)
add_unit_test(test_sessionmanager test_sessionmanager.cpp LocalAIAssistantCore Qt6::Network)
add_unit_test(test_fileparser test_fileparser.cpp LocalAIAssistantCore Qt6::Network)
