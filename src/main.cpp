// main.cpp
// --------
// Entry point for DungeonCrawler. Creates and runs the game.

#include "core/Game.h"

#ifdef __EMSCRIPTEN__
#include "platform/WebConsole.h"
#endif

int main() {
#ifdef __EMSCRIPTEN__
	WebConsole::Install();
#endif
	Game game;
	game.Run();
	return 0;
}
