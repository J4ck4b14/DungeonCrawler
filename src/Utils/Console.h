#pragma once
#include <string>
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>

namespace Console {

// Clear the console screen (cross-platform)
inline void Clear() {
#ifdef _WIN32
	std::system("cls");
#else
	std::system("clear");
#endif
}

// Print a line with a delay (for dramatic pacing)
inline void PrintSlow(const std::string& text, int delayMs = 600) {
	std::cout << text << "\n";
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
}

// Print multiple lines with delay between each
inline void PrintSlowLines(const std::initializer_list<std::string>& lines, int delayMs = 600) {
	for (const auto& line : lines) {
		PrintSlow(line, delayMs);
	}
}

// Pause and wait for user to press enter
inline void WaitForEnter(const std::string& prompt = "  Press Enter to continue...") {
	std::cout << prompt;
	std::cout.flush();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

} // namespace Console
