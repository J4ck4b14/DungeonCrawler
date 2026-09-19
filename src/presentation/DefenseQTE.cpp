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

constexpr int kInputPollMs = 8;
constexpr int kFrameIntervalMs = 32;
constexpr int kFallRows = 10;
constexpr std::size_t kFrameWidth = 70;
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
	case DefenseCueVisibility::Late: return progress >= .45;
	case DefenseCueVisibility::Flicker:
		return std::abs(timingError) <= blockRadiusMs
			|| (elapsedMs / 145) % 2 == 0;
	case DefenseCueVisibility::Sparse:
		return row % 2 == 0 || std::abs(timingError) <= blockRadiusMs;
	}
	return true;
}

std::string LaneRow(const std::array<char, 4>& markers) {
	std::string line;
	for (char marker : markers) {
		line += "|  ";
		line += marker == 0 ? ' ' : marker;
		line += "  | ";
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
	if (band == DefenseCueGrade::Perfect) return ">>> PERFECT WINDOW <<<";
	if (band == DefenseCueGrade::Block)
		return nearestError < 0 ? "BLOCK WINDOW OPEN" : "LATE BLOCK WINDOW";
	return nearestError < 0 ? "READ THE LANES" : "TOO LATE";
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
			marker = marker == 0 ? 'o' : '*';
		}
	}

	TerminalDisplay::MoveCursorHome();
	WriteFrameLine("+====================== REACTIVE GUARD ======================+");
	WriteFrameLine("  " + challenge.attackLabel + "  |  " + challenge.patternLabel);
	WriteFrameLine("  Beat " + std::to_string(cueIndex + 1) + "/"
		+ std::to_string(challenge.cues.size())
		+ "  |  Press A/W/S/D as cues touch the guard line.");
	WriteFrameLine("  Chords accept " + std::to_string(challenge.chordGraceMs)
		+ " ms between near-simultaneous keys.");
	for (const auto& row : field) WriteFrameLine(LaneRow(row));

	std::string guard;
	for (std::size_t lane = 0; lane < 4; ++lane) {
		guard += onGuard[lane]
			? "[ >" + std::string(1, kLanes[lane]) + "< ] "
			: "[  " + std::string(1, kLanes[lane]) + "  ] ";
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
	for (DefenseCueGrade grade : grades) {
		if (grade == DefenseCueGrade::Miss) return DefenseCueGrade::Miss;
		if (grade != DefenseCueGrade::Perfect) allPerfect = false;
	}
	return allPerfect ? DefenseCueGrade::Perfect : DefenseCueGrade::Block;
}

int LatestArrival(const DefenseCue& cue) {
	int result = cue.fallDurationMs;
	for (const DefenseNote& note : cue.notes) {
		if (!note.decoy) result = std::max(result, NoteArrival(cue, note));
	}
	return result;
}

int RequiredInputCount(const DefenseCue& cue) {
	return static_cast<int>(std::count_if(cue.notes.begin(), cue.notes.end(),
		[](const DefenseNote& note) { return !note.decoy; }));
}

} // namespace

namespace DefenseQTE {

DefenseResult Run(const DefenseChallenge& challenge) {
	if (!DefenseRules::IsValidChallenge(challenge)) return DefenseResult::GuardBreak;
	struct CursorGuard {
		CursorGuard() { TerminalDisplay::SetCursorVisible(false); }
		~CursorGuard() { TerminalDisplay::SetCursorVisible(true); }
	} cursorGuard;

	TimedInput::Flush();
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
			if (static_cast<int>(inputs.size()) >= RequiredInputCount(cue)) break;
		}

		const std::vector<DefenseCueGrade> cueGrades = DefenseRules::GradeCueInputs(
			cue, inputs, challenge.blockRadiusMs, challenge.perfectRadiusMs,
			challenge.chordGraceMs);
		sequenceGrades.insert(sequenceGrades.end(), cueGrades.begin(), cueGrades.end());
		const DefenseCueGrade cueGrade = ResolveCueGrade(cueGrades);
		std::cout << "\n  " << GradeName(cueGrade) << "\n";
		std::cout.flush();
		Console::Sleep(cueGrade == DefenseCueGrade::Miss ? 350 : 180);
		if (cueGrade == DefenseCueGrade::Miss) break;
		if (cueIndex + 1 < challenge.cues.size()) Console::Sleep(cue.gapAfterMs);
		TimedInput::Flush();
	}

	return DefenseRules::ResolveSequence(sequenceGrades);
}

} // namespace DefenseQTE
