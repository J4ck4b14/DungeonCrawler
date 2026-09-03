#include "TimedInput.h"

#include <algorithm>
#include <chrono>
#include <thread>

#if defined(__EMSCRIPTEN__)
#include "WebConsole.h"
#elif defined(_WIN32)
#include <conio.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace TimedInput {

void Flush() {
#if defined(__EMSCRIPTEN__)
	while (WaitForKey(0) != 0) {}
#elif defined(_WIN32)
	while (_kbhit()) (void)_getch();
#else
	tcflush(STDIN_FILENO, TCIFLUSH);
#endif
}

int WaitForKey(int timeoutMs) {
	timeoutMs = std::max(0, timeoutMs);
#if defined(__EMSCRIPTEN__)
	return WebConsole::WaitKey(timeoutMs);
#elif defined(_WIN32)
	const auto deadline = std::chrono::steady_clock::now()
		+ std::chrono::milliseconds(timeoutMs);
	do {
		if (_kbhit()) return _getch();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	} while (std::chrono::steady_clock::now() < deadline);
	return 0;
#else
	termios previous{};
	const bool hasTerminal = tcgetattr(STDIN_FILENO, &previous) == 0;
	if (hasTerminal) {
		termios raw = previous;
		raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
		tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	}
	fd_set inputSet;
	FD_ZERO(&inputSet);
	FD_SET(STDIN_FILENO, &inputSet);
	timeval timeout{};
	timeout.tv_sec = timeoutMs / 1000;
	timeout.tv_usec = (timeoutMs % 1000) * 1000;
	const int ready = select(STDIN_FILENO + 1, &inputSet, nullptr, nullptr, &timeout);
	unsigned char key = 0;
	const int result = ready > 0 && read(STDIN_FILENO, &key, 1) == 1 ? key : 0;
	if (hasTerminal) tcsetattr(STDIN_FILENO, TCSANOW, &previous);
	return result;
#endif
}

} // namespace TimedInput
