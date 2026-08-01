# src/CMakeLists.txt

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
)

# Generated compiler-identification sources under an in-tree build directory
# define their own main() and must never become part of the game target.
list(FILTER SRC_FILES EXCLUDE REGEX "/out/")

add_executable(DungeonCrawlerPP ${SRC_FILES})

target_include_directories(DungeonCrawlerPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
