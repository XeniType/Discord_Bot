cmake_minimum_required(VERSION 3.16)
project(SocialDeductionBot CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    dpp
    GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
    GIT_TAG        v10.0.35 
)
FetchContent_MakeAvailable(dpp)

# 1. Point to your source files in the src/ folder
add_executable(bot 
    src/main.cpp 
    src/UI.cpp
    src/CommandHandler.cpp
)

target_link_libraries(bot PRIVATE dpp)

# 2. Tell CMake where to look for local headers AND D++ headers
target_include_directories(bot PRIVATE 
    ${dpp_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --- Post-Build DLL Copying ---
add_custom_command(TARGET bot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dpp>
    $<TARGET_FILE_DIR:bot>
)

add_custom_command(TARGET bot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${dpp_SOURCE_DIR}/win32/bin"
    $<TARGET_FILE_DIR:bot>
    COMMENT "Copying D++ prebuilt third-party DLLs..."
)

add_custom_command(TARGET bot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dpp>
    $<TARGET_FILE_DIR:bot>
    COMMENT "Copying dpp.dll..."
)