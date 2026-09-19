#include "DefenseRules.h"

#include "DefensePatterns.h"
#include "Spell.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>

namespace {

char NormalizeKey(char key) {
	return static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
}

std::string SpeedName(int fallDurationMs) {
	if (fallDurationMs <= 760) return "very fast";
	if (fallDurationMs <= 980) return "fast";
	if (fallDurationMs <= 1280) return "measured";
	return "slow";
}

int RequiredNoteCount(const DefenseCue& cue) {
	return static_cast<int>(std::count_if(cue.notes.begin(), cue.notes.end(),
		[](const DefenseNote& note) { return !note.decoy; }));
}

} // namespace

namespace DefenseRules {

DefenseCueGrade GradeTiming(int timingErrorMs,
	int blockRadiusMs, int perfectRadiusMs) {
	const int absoluteError = std::abs(timingErrorMs);
	if (absoluteError <= std::max(0, perfectRadiusMs)) {
		return DefenseCueGrade::Perfect;
	}
	if (absoluteError <= std::max(0, blockRadiusMs)) {
		return DefenseCueGrade::Block;
	}
	return DefenseCueGrade::Miss;
}

DefenseCueGrade GradeCue(char expectedKey, char pressedKey, int timingErrorMs,
	int blockRadiusMs, int perfectRadiusMs) {
	if (NormalizeKey(expectedKey) != NormalizeKey(pressedKey)) {
		return DefenseCueGrade::Miss;
	}
	return GradeTiming(timingErrorMs, blockRadiusMs, perfectRadiusMs);
}

std::vector<DefenseCueGrade> GradeCueInputs(const DefenseCue& cue,
	const std::vector<DefenseInput>& inputs, int blockRadiusMs,
	int perfectRadiusMs, int chordGraceMs) {
	std::vector<DefenseCueGrade> grades;
	grades.reserve(static_cast<std::size_t>(RequiredNoteCount(cue)));
	std::vector<bool> used(inputs.size(), false);
	std::map<int, int> chordAnchorByOffset;

	for (const DefenseNote& note : cue.notes) {
		if (note.decoy) continue;
		int inputIndex = -1;
		for (std::size_t i = 0; i < inputs.size(); ++i) {
			if (!used[i] && NormalizeKey(inputs[i].key) == NormalizeKey(note.key)) {
				inputIndex = static_cast<int>(i);
				break;
			}
		}
		if (inputIndex < 0) {
			grades.push_back(DefenseCueGrade::Miss);
			continue;
		}

		used[static_cast<std::size_t>(inputIndex)] = true;
		const int pressedAt = inputs[static_cast<std::size_t>(inputIndex)].pressedAtMs;
		int effectivePress = pressedAt;
		const int sameTimeNotes = static_cast<int>(std::count_if(
			cue.notes.begin(), cue.notes.end(), [&](const DefenseNote& candidate) {
				return !candidate.decoy
					&& candidate.arrivalOffsetMs == note.arrivalOffsetMs;
			}));
		if (sameTimeNotes > 1) {
			auto [anchor, inserted] = chordAnchorByOffset.emplace(
				note.arrivalOffsetMs, pressedAt);
			if (!inserted && std::abs(pressedAt - anchor->second)
				<= std::max(0, chordGraceMs)) {
				effectivePress = anchor->second;
			}
		}

		const int arrival = cue.fallDurationMs + note.arrivalOffsetMs;
		grades.push_back(GradeTiming(effectivePress - arrival,
			blockRadiusMs, perfectRadiusMs));
	}

	if (std::any_of(used.begin(), used.end(), [](bool matched) { return !matched; })
		&& !grades.empty()) {
		grades.front() = DefenseCueGrade::Miss;
	}
	return grades;
}

DefenseResult ResolveSequence(const std::vector<DefenseCueGrade>& grades) {
	if (grades.empty()) return DefenseResult::GuardBreak;
	bool allPerfect = true;
	for (DefenseCueGrade grade : grades) {
		if (grade == DefenseCueGrade::Miss) return DefenseResult::GuardBreak;
		if (grade != DefenseCueGrade::Perfect) allPerfect = false;
	}
	return allPerfect ? DefenseResult::PerfectParry : DefenseResult::Block;
}

int DamageAfterDefense(int incomingDamage, DefenseResult result) {
	incomingDamage = std::max(0, incomingDamage);
	switch (result) {
	case DefenseResult::GuardBreak: return incomingDamage;
	case DefenseResult::Block: return incomingDamage / 2;
	case DefenseResult::PerfectParry: return 0;
	}
	return incomingDamage;
}

int SpeedBlockBonusMs(int playerSpeed) {
	playerSpeed = std::max(0, playerSpeed);
	return playerSpeed * DefenseTuning::MaximumSpeedBlockBonusMs
		/ (playerSpeed + 12);
}

int ComplexityTier(int enemyRank) {
	enemyRank = std::clamp(enemyRank, 1, 50);
	if (enemyRank >= 40) return 3;
	if (enemyRank >= 25) return 2;
	if (enemyRank >= 10) return 1;
	return 0;
}

bool IsValidLane(char key) {
	const char normalized = NormalizeKey(key);
	return normalized == 'A' || normalized == 'W'
		|| normalized == 'S' || normalized == 'D';
}

bool IsValidChallenge(const DefenseChallenge& challenge) {
	if (challenge.cues.empty() || challenge.blockRadiusMs < challenge.perfectRadiusMs
		|| challenge.perfectRadiusMs < 0 || challenge.chordGraceMs < 0) return false;
	for (const DefenseCue& cue : challenge.cues) {
		if (cue.notes.empty() || cue.fallDurationMs <= 0 || cue.gapAfterMs < 0
			|| RequiredNoteCount(cue) == 0) return false;
		for (const DefenseNote& note : cue.notes) {
			if (!note.decoy && !IsValidLane(note.key)) return false;
			if (note.arrivalOffsetMs < 0 || note.lanePath.empty()) return false;
			for (char lane : note.lanePath) if (!IsValidLane(lane)) return false;
			if (!note.decoy
				&& NormalizeKey(note.lanePath.back()) != NormalizeKey(note.key)) return false;
		}
	}
	return true;
}

DefenseChallenge BuildChallenge(const TurnAction& action, const Spell* spell,
	EnemyArchetype archetype, int enemyRank, int enemySpeed,
	int enemyStrength, int playerSpeed, int patternVariant) {
	DefenseChallenge challenge = DefensePatterns::Build(
		archetype, enemyRank, action, spell, patternVariant);
	enemyRank = std::clamp(enemyRank, 1, 50);
	const int speedPressure = std::min(180, std::max(0, enemySpeed - 1) * 18);
	const int rankPressure = std::min(140, (enemyRank - 1) * 3);
	const int strengthPressure = std::min(35, std::max(0, enemyStrength) / 4);

	challenge.blockRadiusMs = std::clamp(
		265 - speedPressure / 3 - rankPressure / 3 - strengthPressure
			+ SpeedBlockBonusMs(playerSpeed),
		DefenseTuning::MinimumBlockRadiusMs,
		DefenseTuning::MaximumBlockRadiusMs);
	challenge.perfectRadiusMs = std::clamp(
		78 - speedPressure / 18 - rankPressure / 20,
		DefenseTuning::MinimumPerfectRadiusMs, 78);
	challenge.chordGraceMs = DefenseTuning::ChordGraceMs;

	for (DefenseCue& cue : challenge.cues) {
		cue.fallDurationMs = std::clamp(
			cue.fallDurationMs - speedPressure - rankPressure, 620, 1900);
	}
	return challenge;
}

std::string DescribeChallenge(const DefenseChallenge& challenge) {
	int noteCount = 0;
	bool hasChord = false;
	for (const DefenseCue& cue : challenge.cues) {
		const int required = RequiredNoteCount(cue);
		noteCount += required;
		hasChord = hasChord || required > 1;
	}
	const int fall = challenge.cues.empty() ? 1000
		: challenge.cues.front().fallDurationMs;
	return std::to_string(noteCount) + (noteCount == 1 ? " input, " : " inputs, ")
		+ SpeedName(fall) + (hasChord ? ", includes a chord" : "");
}

} // namespace DefenseRules
