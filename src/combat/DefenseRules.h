#pragma once

#include "CombatTypes.h"
#include "entities/EnemyDefinitions.h"

#include <string>
#include <vector>

struct Spell;

enum class DefenseCueGrade {
	Miss,
	Block,
	Perfect
};

enum class DefenseResult {
	GuardBreak,
	Block,
	PerfectParry
};

enum class DefenseCueVisibility {
	Normal,
	Late,
	Flicker,
	Sparse
};

struct DefenseNote {
	char key = 'W';
	std::vector<char> lanePath = {'W'};
	int arrivalOffsetMs = 0;
	DefenseCueVisibility visibility = DefenseCueVisibility::Normal;
	bool decoy = false;
};

// A cue is one authored beat. Multiple notes with the same arrival offset form
// a chord; different offsets form a staggered chord.
struct DefenseCue {
	std::vector<DefenseNote> notes;
	int fallDurationMs = 1000;
	int gapAfterMs = 240;
};

struct DefenseInput {
	char key = 0;
	int pressedAtMs = 0;
};

struct DefenseChallenge {
	std::string attackLabel;
	std::string patternLabel;
	std::vector<DefenseCue> cues;
	int blockRadiusMs = 220;
	int perfectRadiusMs = 70;
	int chordGraceMs = 65;
};

namespace DefenseTuning {

inline constexpr int MinimumBlockRadiusMs = 135;
inline constexpr int MaximumBlockRadiusMs = 300;
inline constexpr int MinimumPerfectRadiusMs = 45;
inline constexpr int MaximumSpeedBlockBonusMs = 45;
inline constexpr int ChordGraceMs = 65;

} // namespace DefenseTuning

namespace DefenseRules {

DefenseCueGrade GradeTiming(int timingErrorMs,
	int blockRadiusMs, int perfectRadiusMs);
DefenseCueGrade GradeCue(char expectedKey, char pressedKey, int timingErrorMs,
	int blockRadiusMs, int perfectRadiusMs);
std::vector<DefenseCueGrade> GradeCueInputs(const DefenseCue& cue,
	const std::vector<DefenseInput>& inputs, int blockRadiusMs,
	int perfectRadiusMs, int chordGraceMs);
DefenseResult ResolveSequence(const std::vector<DefenseCueGrade>& grades);
int DamageAfterDefense(int incomingDamage, DefenseResult result);

int SpeedBlockBonusMs(int playerSpeed);
int ComplexityTier(int enemyRank);
bool IsValidLane(char key);
bool IsValidChallenge(const DefenseChallenge& challenge);

DefenseChallenge BuildChallenge(const TurnAction& action, const Spell* spell,
	EnemyArchetype archetype, int enemyRank, int enemySpeed,
	int enemyStrength, int playerSpeed, int patternVariant = 0);
std::string DescribeChallenge(const DefenseChallenge& challenge);

} // namespace DefenseRules
