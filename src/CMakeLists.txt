# src/CMakeLists.txt

set(DUNGEON_LIBRARY_SOURCES
    audio/MusicSystem.cpp
    combat/CombatSystem.cpp
	combat/DeathSaveRules.cpp
    combat/DefensePatterns.cpp
    combat/DefenseRules.cpp
    combat/EncounterRules.cpp
    combat/SpellRules.cpp
    core/Bestiary.cpp
    core/DevMode.cpp
    core/Game.cpp
    core/RelicRules.cpp
    core/Relic.cpp
    dungeon/Dungeon.cpp
    dungeon/DungeonTopology.cpp
    dungeon/BreachRules.cpp
    dungeon/DungeonRules.cpp
    dungeon/Perception.cpp
    dungeon/RestSite.cpp
    equipment/Equipment.cpp
    entities/Enemy.cpp
    entities/EnemyBehavior.cpp
    entities/EnemyDefinitions.cpp
    entities/EnemyFactory.cpp
    entities/Entity.cpp
    entities/Player.cpp
    loot/LootGenerator.cpp
    platform/WebConsole.cpp
    platform/ProfileStorage.cpp
    platform/TimedInput.cpp
    platform/TerminalDisplay.cpp
    presentation/CombatDisplay.cpp
    presentation/DefenseQTE.cpp
    presentation/CombatMenu.cpp
    presentation/AudioMenu.cpp
    presentation/ExplorationDisplay.cpp
    presentation/ProfileDisplay.cpp
    presentation/EnemyIntent.cpp
    presentation/EquipmentMenu.cpp
    status/StatusEffect.cpp
    utils/RNG.cpp
    progression/PlayerProfile.cpp
)

add_library(DungeonGame STATIC ${DUNGEON_LIBRARY_SOURCES})
target_include_directories(DungeonGame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(DungeonGame PUBLIC winmm)
endif()

add_executable(DungeonCrawlerPP main.cpp)
target_link_libraries(DungeonCrawlerPP PRIVATE DungeonGame)

if(EMSCRIPTEN)
    # itch-build/index.html expects these exact filenames.
    set_target_properties(DungeonCrawlerPP PROPERTIES
        OUTPUT_NAME "game"
        SUFFIX ".js"
    )

    target_link_options(DungeonCrawlerPP PRIVATE
        "-O3"
        "SHELL:-sASYNCIFY"
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
    )
endif()

if(EMSCRIPTEN AND EXISTS "${CMAKE_SOURCE_DIR}/assets/music")
    target_link_options(DungeonCrawlerPP PRIVATE
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/music@/assets/music")
endif()

function(configure_dungeon_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(DUNGEON_ENABLE_SANITIZERS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /fsanitize=address)
            target_link_options(${target} PRIVATE /INCREMENTAL:NO)
        else()
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

configure_dungeon_target(DungeonGame)
configure_dungeon_target(DungeonCrawlerPP)

if(BUILD_TESTING)
    add_executable(DungeonCrawlerTests ../tests/RulesTests.cpp)
    target_link_libraries(DungeonCrawlerTests PRIVATE DungeonGame)
    configure_dungeon_target(DungeonCrawlerTests)
    add_test(NAME DungeonCrawlerRules COMMAND DungeonCrawlerTests)

	add_executable(DungeonCrawlerValidation ../tests/ValidationReport.cpp)
	target_link_libraries(DungeonCrawlerValidation PRIVATE DungeonGame)
	configure_dungeon_target(DungeonCrawlerValidation)
endif()
