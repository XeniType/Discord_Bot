cmake_minimum_required(VERSION 3.16)
project(SocialDeductionBot CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Pull D++ directly from GitHub (Bypasses vcpkg)
include(FetchContent)
FetchContent_Declare(
    dpp
    GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
    GIT_TAG        v10.0.35 
)
FetchContent_MakeAvailable(dpp)

add_executable(bot main.cpp)

# 1. Link the local 'dpp' target instead of the imported 'dpp::dpp'
target_link_libraries(bot PRIVATE dpp)

# 2. Explicitly tell your bot where the downloaded D++ headers live
target_include_directories(bot PRIVATE ${dpp_SOURCE_DIR}/include)

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