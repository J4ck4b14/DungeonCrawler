#include "DefenseQTE.h"

#include "platform/TimedInput.h"
#include "platform/TerminalDisplay.h"
#include "utils/Console.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kInputPollMs = 4;
constexpr int kFallRows = 10;
constexpr size_t kFrameWidth = 62;

void WriteFrameLine(const std::string& text = {}) {
	std::cout << text;
	if (text.size() < kFrameWidth) {
		std::cout << std::string(kFrameWidth - text.size(), ' ');
	}
	std::cout << '\n';
}

int FallingRow(const DefenseCue& cue, int elapsedMs, int perfectRadiusMs) {
	const int lineArrivalMs = std::max(1, cue.fallDurationMs - perfectRadiusMs);
	return std::clamp(elapsedMs * kFallRows / lineArrivalMs, 0, kFallRows - 1);
}

std::string TimingHint(int timingErrorMs, const DefenseChallenge& challenge) {
	const DefenseCueGrade band = DefenseRules::GradeTiming(timingErrorMs,
		challenge.blockRadiusMs, challenge.perfectRadiusMs);
	if (band == DefenseCueGrade::Perfect) return "                 >>> PARRY NOW <<<";
	if (band == DefenseCueGrade::Block) {
		return timingErrorMs < 0
			? "                    BLOCK WINDOW"
			: "                 LATE BLOCK WINDOW";
	}
	return timingErrorMs < 0 ? "                     GET READY" : "                    TOO LATE";
}

void RenderFrame(const DefenseChallenge& challenge, size_t cueIndex,
	int row, int timingErrorMs, DefenseCueGrade timingBand) {
	const DefenseCue& cue = challenge.cues[cueIndex];
	const bool onGuardLine = timingBand == DefenseCueGrade::Perfect;
	const bool belowGuardLine = timingErrorMs > challenge.perfectRadiusMs;

	TerminalDisplay::MoveCursorHome();
	WriteFrameLine("+==================== REACTIVE GUARD ====================+");
	WriteFrameLine("  " + challenge.attackLabel + "  |  Cue "
		+ std::to_string(cueIndex + 1) + "/" + std::to_string(challenge.cues.size()));
	WriteFrameLine("  Press " + std::string(1, cue.key)
		+ " while it overlaps the guard line.");
	WriteFrameLine();
	for (int currentRow = 0; currentRow < kFallRows; ++currentRow) {
		if (!onGuardLine && !belowGuardLine && currentRow == row) {
			WriteFrameLine("                         [ " + std::string(1, cue.key) + " ]");
		}
		else WriteFrameLine();
	}
	if (onGuardLine) {
		WriteFrameLine("  _________________ [ " + std::string(1, cue.key)
			+ " ] GUARD _________________");
	}
	else {
		WriteFrameLine("  _____________________ GUARD _____________________");
	}
	WriteFrameLine(belowGuardLine
		? "                         [ " + std::string(1, cue.key) + " ]"
		: "");
	WriteFrameLine();
	WriteFrameLine("                         W A S D");
	WriteFrameLine(TimingHint(timingErrorMs, challenge));
	std::cout.flush();
}

const char* GradeName(DefenseCueGrade grade) {
	switch (grade) {
	case DefenseCueGrade::Miss: return "MISS";
	case DefenseCueGrade::Block: return "BLOCK";
	case DefenseCueGrade::Perfect: return "PERFECT";
	}
	return "MISS";
}

} // namespace

namespace DefenseQTE {

DefenseResult Run(const DefenseChallenge& challenge) {
	if (challenge.cues.empty()) return DefenseResult::GuardBreak;
	struct CursorGuard {
		CursorGuard() { TerminalDisplay::SetCursorVisible(false); }
		~CursorGuard() { TerminalDisplay::SetCursorVisible(true); }
	} cursorGuard;
	TimedInput::Flush();
	TerminalDisplay::ClearImmediately();
	std::vector<DefenseCueGrade> grades;
	grades.reserve(challenge.cues.size());

	for (size_t cueIndex = 0; cueIndex < challenge.cues.size(); ++cueIndex) {
		const DefenseCue& cue = challenge.cues[cueIndex];
		const auto start = std::chrono::steady_clock::now();
		const int deadlineMs = cue.fallDurationMs + challenge.blockRadiusMs;
		DefenseCueGrade grade = DefenseCueGrade::Miss;
		bool receivedInput = false;
		int lastRenderedRow = -1;
		DefenseCueGrade lastRenderedBand = DefenseCueGrade::Miss;
		bool hasRendered = false;
		int timingErrorMs = 0;
		int pressedKey = 0;

		while (true) {
			const int elapsedMs = static_cast<int>(std::chrono::duration_cast<
				std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
			timingErrorMs = elapsedMs - cue.fallDurationMs;
			const int row = FallingRow(cue, elapsedMs, challenge.perfectRadiusMs);
			const DefenseCueGrade timingBand = DefenseRules::GradeTiming(timingErrorMs,
				challenge.blockRadiusMs, challenge.perfectRadiusMs);
			if (!hasRendered || row != lastRenderedRow || timingBand != lastRenderedBand) {
				RenderFrame(challenge, cueIndex, row, timingErrorMs, timingBand);
				lastRenderedRow = row;
				lastRenderedBand = timingBand;
				hasRendered = true;
			}
			if (elapsedMs >= deadlineMs) break;

			const int waitMs = std::min(kInputPollMs, deadlineMs - elapsedMs);
			pressedKey = TimedInput::WaitForKey(waitMs);
			if (pressedKey == 0) continue;
			receivedInput = true;
			const int pressedAtMs = static_cast<int>(std::chrono::duration_cast<
				std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
			timingErrorMs = pressedAtMs - cue.fallDurationMs;
			grade = DefenseRules::GradeCue(cue.key, static_cast<char>(pressedKey),
				timingErrorMs,
				challenge.blockRadiusMs, challenge.perfectRadiusMs);
			break;
		}

		grades.push_back(grade);
		std::cout << "\n  ";
		if (!receivedInput) std::cout << "MISSED";
		else if (std::toupper(static_cast<unsigned char>(pressedKey))
			!= std::toupper(static_cast<unsigned char>(cue.key))) {
			std::cout << "WRONG KEY";
		}
		else {
			std::cout << GradeName(grade);
			if (grade != DefenseCueGrade::Perfect) {
				std::cout << " (" << std::abs(timingErrorMs) << " ms "
					<< (timingErrorMs < 0 ? "early" : "late") << ")";
			}
		}
		std::cout << "\n";
		std::cout.flush();
		Console::Sleep(grade == DefenseCueGrade::Miss ? 350 : 180);
		if (grade == DefenseCueGrade::Miss) break;
		if (cueIndex + 1 < challenge.cues.size()) Console::Sleep(cue.gapAfterMs);
		TimedInput::Flush();
	}

	return DefenseRules::ResolveSequence(grades);
}

} // namespace DefenseQTE
