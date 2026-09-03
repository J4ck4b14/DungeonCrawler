#include "TerminalDisplay.h"

#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace TerminalDisplay {

void ClearImmediately() {
#ifdef _WIN32
	const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	if (output != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(output, &info)) {
		const DWORD cells = static_cast<DWORD>(info.dwSize.X) * info.dwSize.Y;
		const COORD origin{0, 0};
		DWORD written = 0;
		FillConsoleOutputCharacterA(output, ' ', cells, origin, &written);
		FillConsoleOutputAttribute(output, info.wAttributes, cells, origin, &written);
		SetConsoleCursorPosition(output, origin);
		return;
	}
#endif
	std::cout << "\x1b[2J\x1b[H";
}

void MoveCursorHome() {
#ifdef _WIN32
	const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	if (output != INVALID_HANDLE_VALUE
		&& SetConsoleCursorPosition(output, COORD{0, 0})) {
		return;
	}
#endif
	std::cout << "\x1b[H";
}

void SetCursorVisible(bool visible) {
#ifdef _WIN32
	const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursor{};
	if (output != INVALID_HANDLE_VALUE && GetConsoleCursorInfo(output, &cursor)) {
		cursor.bVisible = visible ? TRUE : FALSE;
		SetConsoleCursorInfo(output, &cursor);
		return;
	}
#endif
	std::cout << (visible ? "\x1b[?25h" : "\x1b[?25l");
	std::cout.flush();
}

} // namespace TerminalDisplay
