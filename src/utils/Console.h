#pragma once

#include "combat/DeathSaveRules.h"
#include "platform/TimedInput.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#ifdef __EMSCRIPTEN__
#include "platform/WebConsole.h"
#endif

namespace Console {

inline void Clear() {
#if defined(__EMSCRIPTEN__)
	std::cout << "\x1b[2J\x1b[3J\x1b[H";
	std::cout.flush();
#elif defined(_WIN32)
	std::system("cls");
#else
	std::system("clear");
#endif
}

inline void Sleep(int ms) {
#ifdef __EMSCRIPTEN__
	WebConsole::Sleep(ms);
#else
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

inline void PrintSlow(const std::string& text, int delayMs = 600) {
	std::cout << text << "\n";
	std::cout.flush();
	Sleep(delayMs);
}

inline void PrintSlowLines(const std::initializer_list<std::string>& lines,
	int delayMs = 600) {
	for (const auto& line : lines) PrintSlow(line, delayMs);
}

inline void WaitForEnter(
	const std::string& prompt = "  Press Enter to continue...") {
#ifdef __EMSCRIPTEN__
	while (std::cin.rdbuf()->in_avail() > 0) std::cin.get();
	std::cout << prompt;
	std::cout.flush();
	std::cin.get();
	while (std::cin.rdbuf()->in_avail() > 0) std::cin.get();
#else
	if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() == '\n') {
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	std::cout << prompt;
	std::cout.flush();
	std::cin.get();
#endif
}

// Web, Windows, Linux, and macOS share one real-time input path. Unknown
// targets fail explicitly instead of replacing player execution with RNG.
inline bool HeartbeatQTE(const std::vector<int>& sequence, int beatMs, int windowMs) {
	if (!TimedInput::IsRealtimeSupported()) {
		std::cout << "  Timed heartbeat input is unavailable on this platform.\n";
		return false;
	}
	TimedInput::Flush();
	for (int digit : sequence) {
		Sleep(beatMs);
		std::cout << "\r                                        \r"
			<< "    ...thump...  [ " << digit << " ]  ";
		std::cout.flush();
		TimedInput::Flush();
		if (!DeathSaveRules::KeyMatches(digit, TimedInput::WaitForKey(windowMs))) {
			std::cout << " X\n";
			std::cout.flush();
			Sleep(300);
			return false;
		}
		std::cout << " *";
		std::cout.flush();
	}
	std::cout << "\n";
	return true;
}

} // namespace Console
