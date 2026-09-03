// Console.h
// ----------
// Utility functions for console output control:
//   - Clear():          Clears the terminal screen.
//   - PrintSlow():      Prints with dramatic pacing delay.
//   - PrintSlowLines(): Multiple lines with delay.
//   - WaitForEnter():   Pauses until Enter is pressed.
//   - HeartbeatQTE():   Death-save: rhythmic number sequence the player must match.

#pragma once
#include <string>
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

#ifdef _WIN32
#include <conio.h>
#endif

#ifdef __EMSCRIPTEN__
#include "platform/WebConsole.h"
#endif

namespace Console {

inline void Clear() {
#if defined(__EMSCRIPTEN__)
	// ANSI: clear screen + scrollback, cursor home (handled by xterm.js)
	std::cout << "\x1b[2J\x1b[3J\x1b[H";
	std::cout.flush();
#elif defined(_WIN32)
	std::system("cls");
#else
	std::system("clear");
#endif
}

// Sleep that is safe on every platform (never blocks the browser tab).
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

inline void PrintSlowLines(const std::initializer_list<std::string>& lines, int delayMs = 600) {
	for (const auto& line : lines) {
		PrintSlow(line, delayMs);
	}
}

inline void WaitForEnter(const std::string& prompt = "  Press Enter to continue...") {
#ifdef __EMSCRIPTEN__
	// Discard any leftover buffered input, then wait for a fresh line.
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

// Heartbeat QTE death-save.
// Shows a sequence of digits (1-3) one or two at a time on a rhythmic beat.
// Player must press the matching key within the time window for each beat.
// timeWindowMs shrinks with each death save (pass deathSaveCount to scale).
// Returns true if the player matches the entire sequence.
//
// sequence: vector of digits (1-3) to display
// beatMs:   base time between beats (heartbeat rhythm)
// windowMs: time the player has to press the correct key per beat
inline bool HeartbeatQTE(const std::vector<int>& sequence, int beatMs, int windowMs) {
#if defined(__EMSCRIPTEN__)
	// Real-time QTE in the browser: same rules as the Windows version.
	for (size_t i = 0; i < sequence.size(); ++i) {
		int digit = sequence[i];

		Sleep(beatMs);

		std::cout << "\r                                        \r";
		std::cout << "    ...thump...  [ " << digit << " ]  ";
		std::cout.flush();

		int ch = WebConsole::WaitKey(windowMs);
		if (ch != ('0' + digit)) {
			// Timeout (0) or wrong key: fail
			std::cout << " X";
			std::cout.flush();
			Sleep(300);
			std::cout << "\n";
			return false;
		}
		std::cout << " *";
		std::cout.flush();
	}
	std::cout << "\n";
	return true;

#elif defined(_WIN32)
	// Flush any buffered keypresses
	while (_kbhit()) (void)_getch();

	for (size_t i = 0; i < sequence.size(); ++i) {
		int digit = sequence[i];

		// Display the beat — dramatic heartbeat pause then show the number
		std::this_thread::sleep_for(std::chrono::milliseconds(beatMs));

		// Show the digit with a heartbeat-style display
		std::cout << "\r                                        \r"; // Clear line
		std::cout << "    ...thump...  [ " << digit << " ]  ";
		std::cout.flush();

		// Flush any early keypresses
		while (_kbhit()) (void)_getch();

		// Wait for the correct keypress within the window
		auto start = std::chrono::steady_clock::now();
		bool matched = false;

		while (true) {
			auto elapsed = std::chrono::steady_clock::now() - start;
			int elapsedMs = static_cast<int>(
				std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
			if (elapsedMs >= windowMs) {
				break; // Time's up for this beat
			}
			if (_kbhit()) {
				int ch = _getch();
				// Check if the pressed key matches the digit ('1', '2', or '3')
				if (ch == ('0' + digit)) {
					matched = true;
					std::cout << " *";
					std::cout.flush();
					break;
				}
				else {
					// Wrong key — fail immediately
					std::cout << " X";
					std::cout.flush();
					std::this_thread::sleep_for(std::chrono::milliseconds(300));
					std::cout << "\n";
					return false;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		if (!matched) {
			// Didn't press anything in time
			std::cout << " X";
			std::cout.flush();
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			std::cout << "\n";
			return false;
		}
	}

	std::cout << "\n";
	return true; // All beats matched!

#else
	// Non-Windows fallback: no _kbhit available, so use a d20 roll.
	// The windowMs parameter encodes difficulty — lower window = more death saves used.
	// Threshold: succeed on 10 + deathCount.
	// We derive deathCount from the window: base is 1000, -100 per save.
	int deathCount = (1000 - windowMs) / 100;
	int threshold = 10 + deathCount;
	if (threshold > 19) threshold = 19; // Always a chance on nat 20

	// Dramatic d20 roll
	std::cout << "  Your fate hangs by a thread...\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	std::cout << "  Rolling for survival (need " << threshold << "+)...";
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(1200));

	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_int_distribution<int> dist(1, 20);
	int roll = dist(eng);

	std::cout << " [ " << roll << " ]\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(600));

	if (roll >= threshold) {
		std::cout << "  ** SUCCESS! **\n";
		return true;
	}
	else {
		std::cout << "  ...not enough.\n";
		return false;
	}
#endif
}

} // namespace Console
