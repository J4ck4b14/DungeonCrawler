// Entry point for the native and WebAssembly builds.

#include "core/DevMode.h"
#include "core/Game.h"

#include <string_view>

#ifdef __EMSCRIPTEN__
#include "platform/WebConsole.h"
#endif

int main(int argc, char* argv[]) {
#ifdef __EMSCRIPTEN__
	WebConsole::Install();
#endif

	for (int i = 1; i < argc; ++i) {
		if (std::string_view(argv[i]) == "--dev") {
			DevMode::Enable();
		}
	}

	Game game;
	game.Run();
	return 0;
}
