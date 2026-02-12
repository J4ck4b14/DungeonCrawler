# src/CMakeLists.txt

file(GLOB_RECURSE SRC_FILES
    "*.cpp"
    "*.h"
)

# Ensure DevMode implementation is included even if glob misses it
list(APPEND SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/core/DevMode.cpp")

add_executable(DungeonCrawlerPP ${SRC_FILES})

target_include_directories(DungeonCrawlerPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})