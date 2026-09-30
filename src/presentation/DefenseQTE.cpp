#include "DefenseQTE.h"

#include "combat/DefenseRules.h"
#include "platform/TimedInput.h"
#include "platform/TerminalDisplay.h"
#include "utils/Console.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Rendering is intentionally separate from grading: the QTE records key/time
// samples, while DefenseRules decides what those samples mean. This keeps the
// timing model testable without a terminal or real-time sleeps.
constexpr int kInputPollMs = 8;
constexpr int kFrameIntervalMs = 32;
constexpr int kFallRows = 12;
constexpr std::size_t kFrameWidth = 74;
constexpr char kLanes[] = {'A', 'W', 'S', 'D'};

void WriteFrameLine(const std::string& text = {}) {
	std::cout << text;
	if (text.size() < kFrameWidth) {
		std::cout << std::string(kFrameWidth - text.size(), ' ');
	}
	std::cout << '\n';
}

char NormalizeKey(char key) {
	return static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
}

int LaneIndex(char lane) {
	lane = NormalizeKey(lane);
	for (int i = 0; i < 4; ++i) if (kLanes[i] == lane) return i;
	return 0;
}

int NoteArrival(const DefenseCue& cue, const DefenseNote& note) {
	return cue.fallDurationMs + note.arrivalOffsetMs;
}

double NoteProgress(const DefenseCue& cue, const DefenseNote& note, int elapsedMs) {
	return std::clamp(static_cast<double>(elapsedMs)
		/ std::max(1, NoteArrival(cue, note)), 0.0, 1.0);
}

int FallingRow(const DefenseCue& cue, const DefenseNote& note,
	int elapsedMs, int perfectRadiusMs) {
	const int lineArrivalMs = std::max(1, NoteArrival(cue, note) - perfectRadiusMs);
	return std::clamp(elapsedMs * kFallRows / lineArrivalMs, 0, kFallRows - 1);
}

char CurrentLane(const DefenseCue& cue, const DefenseNote& note, int elapsedMs) {
	if (note.lanePath.empty()) return note.key;
	const double progress = NoteProgress(cue, note, elapsedMs);
	const int index = std::min(static_cast<int>(note.lanePath.size()) - 1,
		static_cast<int>(progress * note.lanePath.size()));
	return note.lanePath[static_cast<std::size_t>(index)];
}

bool IsVisible(const DefenseCue& cue, const DefenseNote& note,
	int elapsedMs, int row, int blockRadiusMs) {
	const int timingError = elapsedMs - NoteArrival(cue, note);
	if (timingError > blockRadiusMs) return false;
	const double progress = NoteProgress(cue, note, elapsedMs);
	if (note.decoy) return progress < .68;
	switch (note.visibility) {
	case DefenseCueVisibility::Normal: return true;
	case DefenseCueVisibility::Late: return progress >= .38;
	case DefenseCueVisibility::Flicker:
		return std::abs(timingError) <= blockRadiusMs
			|| (elapsedMs / 165) % 2 == 0;
	case DefenseCueVisibility::Sparse:
		return row % 2 == 0 || std::abs(timingError) <= blockRadiusMs;
	}
	return true;
}

std::string LaneRow(const std::array<char, 4>& markers) {
	std::string line;
	for (char marker : markers) {
		line += "|   ";
		line += marker == 0 ? ' ' : marker;
		line += "   | ";
	}
	return line;
}

std::string TimingHint(const DefenseCue& cue, int elapsedMs,
	const DefenseChallenge& challenge) {
	int nearestError = 1000000;
	for (const DefenseNote& note : cue.notes) {
		if (note.decoy) continue;
		const int error = elapsedMs - NoteArrival(cue, note);
		if (std::abs(error) < std::abs(nearestError)) nearestError = error;
	}
	const DefenseCueGrade band = DefenseRules::GradeTiming(nearestError,
		challenge.blockRadiusMs, challenge.perfectRadiusMs);
	if (band == DefenseCueGrade::Perfect) return ">>> GUARD NOW <<<";
	if (band == DefenseCueGrade::Block)
		return nearestError < 0 ? "BLOCK WINDOW OPEN" : "LATE - STILL BLOCKABLE";
	return nearestError < 0 ? "TRACK THE LETTER" : "TOO LATE";
}

bool HasChord(const DefenseChallenge& challenge) {
	for (const DefenseCue& cue : challenge.cues) {
		int required = 0;
		for (const DefenseNote& note : cue.notes) required += !note.decoy;
		if (required > 1) return true;
	}
	return false;
}

void RenderReady(const DefenseChallenge& challenge) {
	TerminalDisplay::ClearImmediately();
	WriteFrameLine("+======================== REACTIVE GUARD ========================+");
	WriteFrameLine("  " + challenge.attackLabel + "  |  " + challenge.patternLabel);
	WriteFrameLine();
	WriteFrameLine("  Track the LETTER. Press that letter when it reaches GUARD.");
	WriteFrameLine("  A wrong press costs one beat; it does not erase good defenses.");
	if (HasChord(challenge)) {
		WriteFrameLine("  Multiple letters together form a chord - press them together.");
	}
	else WriteFrameLine();
	WriteFrameLine();
	WriteFrameLine("                           BRACE");
	std::cout.flush();
}

void RenderFrame(const DefenseChallenge& challenge, std::size_t cueIndex,
	int elapsedMs) {
	const DefenseCue& cue = challenge.cues[cueIndex];
	std::array<std::array<char, 4>, kFallRows> field{};
	std::array<bool, 4> onGuard{};

	for (const DefenseNote& note : cue.notes) {
		const int arrival = NoteArrival(cue, note);
		const int timingError = elapsedMs - arrival;
		const int row = FallingRow(cue, note, elapsedMs,
			challenge.perfectRadiusMs);
		if (!IsVisible(cue, note, elapsedMs, row,
			challenge.blockRadiusMs)) continue;
		const int lane = LaneIndex(CurrentLane(cue, note, elapsedMs));
		if (!note.decoy && std::abs(timingError) <= challenge.perfectRadiusMs) {
			onGuard[static_cast<std::size_t>(LaneIndex(note.key))] = true;
		}
		else if (timingError < -challenge.perfectRadiusMs) {
			char& marker = field[static_cast<std::size_t>(row)][static_cast<std::size_t>(lane)];
			const char cueMarker = note.decoy ? '?' : NormalizeKey(note.key);
			marker = marker == 0 ? cueMarker : '*';
		}
	}

	TerminalDisplay::MoveCursorHome();
	WriteFrameLine("+======================== REACTIVE GUARD ========================+");
	WriteFrameLine("  " + challenge.attackLabel + "  |  " + challenge.patternLabel);
	WriteFrameLine("  Beat " + std::to_string(cueIndex + 1) + "/"
		+ std::to_string(challenge.cues.size())
		+ "  |  Press the shown LETTER when it reaches GUARD.");
	WriteFrameLine("          A         W         S         D");
	for (const auto& row : field) WriteFrameLine(LaneRow(row));

	std::string guard;
	for (std::size_t lane = 0; lane < 4; ++lane) {
		guard += onGuard[lane]
			? "[  >" + std::string(1, kLanes[lane]) + "<  ] "
			: "[   " + std::string(1, kLanes[lane]) + "   ] ";
	}
	WriteFrameLine(guard + "<- GUARD");
	WriteFrameLine();
	WriteFrameLine("  " + TimingHint(cue, elapsedMs, challenge));
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

DefenseCueGrade ResolveCueGrade(const std::vector<DefenseCueGrade>& grades) {
	if (grades.empty()) return DefenseCueGrade::Miss;
	bool allPerfect = true;
	bool allMiss = true;
	for (DefenseCueGrade grade : grades) {
		allPerfect = allPerfect && grade == DefenseCueGrade::Perfect;
		allMiss = allMiss && grade == DefenseCueGrade::Miss;
	}
	if (allPerfect) return DefenseCueGrade::Perfect;
	if (allMiss) return DefenseCueGrade::Miss;
	return DefenseCueGrade::Block;
}

int LatestArrival(const DefenseCue& cue) {
	int result = cue.fallDurationMs;
	for (const DefenseNote& note : cue.notes) {
		if (!note.decoy) result = std::max(result, NoteArrival(cue, note));
	}
	return result;
}

} // namespace

namespace DefenseQTE {

DefenseOutcome Run(const DefenseChallenge& challenge) {
	if (!DefenseRules::IsValidChallenge(challenge)) return {};

	// The cursor must come back even if the terminal input loop exits early.
	struct CursorGuard {
		CursorGuard() { TerminalDisplay::SetCursorVisible(false); }
		~CursorGuard() { TerminalDisplay::SetCursorVisible(true); }
	} cursorGuard;

	TimedInput::Flush();
	RenderReady(challenge);
	Console::Sleep(challenge.readyDurationMs);
	TerminalDisplay::ClearImmediately();

	std::vector<DefenseCueGrade> sequenceGrades;
	for (std::size_t cueIndex = 0; cueIndex < challenge.cues.size(); ++cueIndex) {
		const DefenseCue& cue = challenge.cues[cueIndex];
		const int deadlineMs = LatestArrival(cue) + challenge.blockRadiusMs;
		const auto start = std::chrono::steady_clock::now();
		std::vector<DefenseInput> inputs;
		int lastFrameMs = -kFrameIntervalMs;

		while (true) {
			const int elapsedMs = static_cast<int>(std::chrono::duration_cast<
				std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
			if (elapsedMs - lastFrameMs >= kFrameIntervalMs) {
				RenderFrame(challenge, cueIndex, elapsedMs);
				lastFrameMs = elapsedMs;
			}
			if (elapsedMs >= deadlineMs) break;

			const int key = TimedInput::WaitForKey(std::min(kInputPollMs,
				deadlineMs - elapsedMs));
			if (key == 0 || !DefenseRules::IsValidLane(static_cast<char>(key))) continue;
			const int pressedAtMs = static_cast<int>(std::chrono::duration_cast<
				std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
			inputs.push_back({NormalizeKey(static_cast<char>(key)), pressedAtMs});
		}

		const std::vector<DefenseCueGrade> cueGrades = DefenseRules::GradeCueInputs(
			cue, inputs, challenge.blockRadiusMs, challenge.perfectRadiusMs,
			challenge.chordGraceMs);
		sequenceGrades.insert(sequenceGrades.end(), cueGrades.begin(), cueGrades.end());
		const DefenseCueGrade cueGrade = ResolveCueGrade(cueGrades);
		std::cout << "\n  " << GradeName(cueGrade) << "\n";
		std::cout.flush();
		Console::Sleep(cueGrade == DefenseCueGrade::Miss ? 260 : 160);

		// A missed beat is recoverable. The next authored beat still arrives, which
		// makes the guard read as a sequence of impacts rather than one hidden pass/fail test.
		if (cueIndex + 1 < challenge.cues.size()) Console::Sleep(cue.gapAfterMs);
		TimedInput::Flush();
	}

	return DefenseRules::ResolveOutcome(sequenceGrades);
}

} // namespace DefenseQTE
