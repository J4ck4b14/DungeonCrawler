# Gather sources (keep CONFIGURE_DEPENDS so IDE regenerates when files change)
file(GLOB_RECURSE GAME_CPP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Ensure DevMode implementation is included even if glob misses it
list(APPEND GAME_CPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/core/DevMode.cpp")

# Create a static library for shared code
add_library(game_core STATIC ${GAME_CPP_SOURCES})

target_include_directories(game_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(game_core PUBLIC cxx_std_20)

# Native executable (regular .exe / *nix binary)
add_executable(DungeonCrawlerPP "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(DungeonCrawlerPP PRIVATE game_core)

target_compile_features(DungeonCrawlerPP PRIVATE cxx_std_20)

target_include_directories(DungeonCrawlerPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Emscripten web target: build only when using the Emscripten toolchain
if(EMSCRIPTEN)
    message(STATUS "Configuring Emscripten web target")

    add_executable(dungeoncrawler_wasm "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    target_link_libraries(dungeoncrawler_wasm PRIVATE game_core)
    target_compile_features(dungeoncrawler_wasm PRIVATE cxx_std_20)
    target_include_directories(dungeoncrawler_wasm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Place generated web files in the site's web/game directory at source tree root
    set(WEB_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/web/game")
    file(MAKE_DIRECTORY "${WEB_OUTPUT_DIR}")
    set_target_properties(dungeoncrawler_wasm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${WEB_OUTPUT_DIR}"
        OUTPUT_NAME "dungeoncrawler_wasm"
        PREFIX ""
        SUFFIX ".html"
    )

    target_link_options(dungeoncrawler_wasm PRIVATE
        "-s" "ALLOW_MEMORY_GROWTH=1"
        "-s" "EXIT_RUNTIME=1"
        "-s" "ENVIRONMENT=web"
    )

    message(STATUS "Emscripten web target will output to: ${WEB_OUTPUT_DIR}")
endif()