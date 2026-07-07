// WebConsole.cpp
// --------------
// Implementation of the web platform layer.
//
// How it works:
//   - Output: a custom streambuf sends every byte written to std::cout
//     straight to the JS function `termWrite`, which feeds xterm.js.
//     No line buffering, so prompts without '\n' appear immediately.
//   - Input: a custom streambuf whose underflow() calls an EM_ASYNC_JS
//     function. Asyncify suspends the whole C++ call stack while the
//     browser waits for the player to type a line, then resumes it.
//     This lets blocking std::cin code run unmodified in a browser.

#ifdef __EMSCRIPTEN__

#include "WebConsole.h"

#include <emscripten.h>
#include <iostream>
#include <streambuf>
#include <string>
#include <cstring>
#include <cstdlib>

// ---- JS bridge functions ----

EM_JS(void, js_term_write, (const char* text), {
	if (typeof termWrite === 'function') termWrite(UTF8ToString(text));
});

// Suspends C++ execution until the player submits a line in the terminal.
// Returns a malloc'd UTF-8 string that we must free.
EM_ASYNC_JS(char*, js_term_read_line, (), {
	const line = await termReadLine();
	const len = lengthBytesUTF8(line) + 1;
	const ptr = _malloc(len);
	stringToUTF8(line, ptr, len);
	return ptr;
});

// Suspends until a single key is pressed, or the timeout elapses.
// Returns the char code of the key, or 0 on timeout.
EM_ASYNC_JS(int, js_term_wait_key, (int timeoutMs), {
	return await termWaitKey(timeoutMs);
});

// ---- Output streambuf: every write goes straight to the terminal ----

namespace {

class TermOutBuf : public std::streambuf {
protected:
	int overflow(int c) override {
		if (c != EOF) {
			char ch = static_cast<char>(c);
			char buf[2] = { ch, '\0' };
			// xterm.js needs \r\n for a newline
			if (ch == '\n') js_term_write("\r\n");
			else            js_term_write(buf);
		}
		return c;
	}

	std::streamsize xsputn(const char* s, std::streamsize n) override {
		// Translate \n -> \r\n for xterm.js
		std::string out;
		out.reserve(static_cast<size_t>(n) + 8);
		for (std::streamsize i = 0; i < n; ++i) {
			if (s[i] == '\n') out += "\r\n";
			else              out += s[i];
		}
		js_term_write(out.c_str());
		return n;
	}
};

// ---- Input streambuf: underflow() asks the browser for a line ----

class TermInBuf : public std::streambuf {
public:
	TermInBuf() { setg(buffer_, buffer_, buffer_); }

protected:
	int underflow() override {
		if (gptr() < egptr()) {
			return traits_type::to_int_type(*gptr());
		}

		// Ask the browser for one line of input (Asyncify suspends here)
		char* line = js_term_read_line();
		std::string s = line ? line : "";
		std::free(line);

		// The game expects lines terminated by '\n' (as std::getline
		// and operator>> would see from a real terminal).
		s += '\n';

		if (s.size() > sizeof(buffer_)) {
			s.resize(sizeof(buffer_));
			buffer_[sizeof(buffer_) - 1] = '\n';
		}
		std::memcpy(buffer_, s.data(), s.size());
		setg(buffer_, buffer_, buffer_ + s.size());
		return traits_type::to_int_type(*gptr());
	}

private:
	char buffer_[4096];
};

} // namespace

// ---- Public API ----

namespace WebConsole {

void Install() {
	static TermOutBuf outBuf;
	static TermInBuf inBuf;
	std::cout.rdbuf(&outBuf);
	std::cerr.rdbuf(&outBuf);
	std::cin.rdbuf(&inBuf);
	// No sync with C stdio; everything goes through the streambufs.
	std::ios::sync_with_stdio(false);
}

int WaitKey(int timeoutMs) {
	return js_term_wait_key(timeoutMs);
}

void Sleep(int ms) {
	emscripten_sleep(ms);
}

} // namespace WebConsole

#endif // __EMSCRIPTEN__
