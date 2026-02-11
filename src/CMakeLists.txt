# src/CMakeLists.txt

file(GLOB_RECURSE SRC_FILES
    "*.cpp"
    "*.h"
)

add_executable(DungeonCrawlerPP ${SRC_FILES})

target_include_directories(DungeonCrawlerPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})