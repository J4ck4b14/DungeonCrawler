# src/CMakeLists.txt

file(GLOB_RECURSE SRC_FILES
    "*.cpp"
)

add_executable(DungeonCrawlerPP ${SRC_FILES} "entities/Entity.h" "entities/Entity.cpp" "entities/Player.cpp" "entities/Enemy.h" "entities/Enemy.cpp")

target_include_directories(DungeonCrawlerPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})