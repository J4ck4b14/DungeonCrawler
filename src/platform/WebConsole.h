// WebConsole.h
// ------------
// Web (Emscripten) platform layer.
// Redirects std::cin / std::cout through the browser terminal (xterm.js)
// so the entire game runs unmodified in a web page.
//
// Only active when compiled with Emscripten (__EMSCRIPTEN__).

#pragma once

#ifdef __EMSCRIPTEN__

namespace WebConsole {

// Install custom stream buffers on std::cin / std::cout.
// Call once at the start of main().
void Install();

// Wait for a single keypress (returns the char code) or 0 on timeout.
// Used by the HeartbeatQTE death-save minigame.
int WaitKey(int timeoutMs);

// Sleep without blocking the browser main thread (Asyncify).
void Sleep(int ms);

} // namespace WebConsole

#endif // __EMSCRIPTEN__
