# DungeonCrawler/CMakeLists.txt

cmake_minimum_required(VERSION 3.21)
project(DungeonCrawlerPP VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
option(DUNGEON_ENABLE_SANITIZERS "Enable the sanitizers supported by the active compiler" OFF)

add_subdirectory(src)
