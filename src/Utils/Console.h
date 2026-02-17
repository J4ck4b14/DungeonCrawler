// Console.h
// Cross-platform console utilities.
// - Native .exe: preserves existing behavior.
// - Web (Emscripten): prints into a DOM element with id="output" and captures keypresses for QTE.
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

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifdef _WIN32
#include <conio.h>
#endif

namespace Console {

// Clear the screen / output area
inline void Clear() {
#ifdef __EMSCRIPTEN__
	EM_ASM({
		var el = document.getElementById('output');
		if (el) el.textContent = '';
		else console.clear();
	});
#else
#ifdef _WIN32
	std::system("cls");
#else
	std::system("clear");
#endif
#endif
}

// Print a line then pause for delayMs milliseconds
inline void PrintSlow(const std::string& text, int delayMs = 600) {
#ifdef __EMSCRIPTEN__
	EM_ASM({
		var s = UTF8ToString($0);
		var el = document.getElementById('output');
		if (el) {
			el.textContent += s + "\n";
			el.scrollTop = el.scrollHeight;
		} else {
			console.log(s);
		}
	}, text.c_str());
	// Keep a small delay so pacing feels similar to console version.
	std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
#else
	std::cout << text << "\n";
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
#endif
}

inline void PrintSlowLines(const std::initializer_list<std::string>& lines, int delayMs = 600) {
	for (const auto& line : lines) {
		PrintSlow(line, delayMs);
	}
}

// Blocking "Press Enter" behaviour.
// On web we use prompt()/alert() to emulate blocking input so existing game flow continues.
inline void WaitForEnter(const std::string& prompt = "  Press Enter to continue...") {
#ifdef __EMSCRIPTEN__
	// Use a blocking prompt so emscripten execution pauses until user responds.
	EM_ASM({
		var p = UTF8ToString($0);
		// Use prompt for better UX (shows input box). If prompt is unavailable, fallback to alert.
		try {
			prompt(p);
		} catch (e) {
			alert(p);
		}
	}, prompt.c_str());
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
// Returns true if the player matches the entire sequence.
//
// sequence: vector of digits (1-3) to display
// beatMs:   base time between beats (heartbeat rhythm)
// windowMs: time the player has to press the correct key per beat
inline bool HeartbeatQTE(const std::vector<int>& sequence, int beatMs, int windowMs) {
#ifdef __EMSCRIPTEN__
	// Install a temporary key listener that stores the last numeric key pressed (1-3) in window.__qte_last.
	EM_ASM({
		window.__qte_last = -1;
		// create a unique listener so we can remove it reliably
		window.__qte_listener = function(e) {
			if (e.key >= '1' && e.key <= '3') {
				window.__qte_last = e.key.charCodeAt(0);
			}
		};
		window.addEventListener('keydown', window.__qte_listener);
	});

	// Ensure any previous buffered key is cleared
	EM_ASM({ window.__qte_last = -1; });

	for (size_t i = 0; i < sequence.size(); ++i) {
		int digit = sequence[i];

		// Heartbeat pause
		std::this_thread::sleep_for(std::chrono::milliseconds(beatMs));

		// Flash the digit briefly (not the whole sequence). Append to output so it mirrors the console.
		std::string digitStr = std::to_string(digit);
		EM_ASM({
			var s = UTF8ToString($0);
			var el = document.getElementById('output');
			if (el) {
				el.textContent += "    ...thump...  [ " + s + " ]  \n";
				el.scrollTop = el.scrollHeight;
			} else {
				console.log("...thump...  [ " + s + " ]");
			}
		}, digitStr.c_str());

		// Reset buffered key before waiting
		EM_ASM({ window.__qte_last = -1; });

		// Wait for keypress within windowMs, polling the JS variable
		auto start = std::chrono::steady_clock::now();
		bool matched = false;

		while (true) {
			auto elapsed = std::chrono::steady_clock::now() - start;
			int elapsedMs = static_cast<int>(
				std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
			if (elapsedMs >= windowMs) {
				break; // time's up for this beat
			}

			// poll last key
			int jsKey = EM_ASM_INT({
				return (typeof window.__qte_last === 'number') ? window.__qte_last : -1;
			});
			if (jsKey != -1) {
				// normalize and clear immediately
				EM_ASM({ window.__qte_last = -1; });
				if (jsKey == ('0' + digit)) {
					// correct key
					matched = true;
					EM_ASM({
						var el = document.getElementById('output');
						if (el) { el.textContent += " *\n"; el.scrollTop = el.scrollHeight; }
						else console.log(" *");
					});
					break;
				} else {
					// wrong key -> immediate fail
					EM_ASM({
						var el = document.getElementById('output');
						if (el) { el.textContent += " X\n"; el.scrollTop = el.scrollHeight; }
						else console.log(" X");
					});
					// remove listener and return
					EM_ASM({
						if (window.__qte_listener) {
							window.removeEventListener('keydown', window.__qte_listener);
							window.__qte_listener = null;
						}
						window.__qte_last = -1;
					});
					return false;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		if (!matched) {
			EM_ASM({
				var el = document.getElementById('output');
				if (el) { el.textContent += " X\n"; el.scrollTop = el.scrollHeight; }
				else console.log(" X");
			});
			EM_ASM({ 
				if (window.__qte_listener) {
					window.removeEventListener('keydown', window.__qte_listener);
					window.__qte_listener = null;
				}
				window.__qte_last = -1;
			});
			return false;
		}
	}

	// Remove listener and clean up
	EM_ASM({
		if (window.__qte_listener) {
			window.removeEventListener('keydown', window.__qte_listener);
			window.__qte_listener = null;
		}
		window.__qte_last = -1;
	});

	return true;

#else
#ifdef _WIN32
	// Windows console implementation using kbhit/getch (keeps older behavior)
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
	// Non-Windows fallback: preserve existing d20/random fallback for terminals lacking kbhit
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
#endif
}

} // namespace Console
