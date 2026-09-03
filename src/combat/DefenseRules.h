#pragma once

#include "CombatTypes.h"

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

struct DefenseCue {
	char key = 'W';
	int fallDurationMs = 1000;
	int gapAfterMs = 400;
};

struct DefenseChallenge {
	std::string attackLabel;
	std::vector<DefenseCue> cues;
	int blockRadiusMs = 220;
	int perfectRadiusMs = 70;
};

namespace DefenseRules {

DefenseCueGrade GradeTiming(int timingErrorMs,
	int blockRadiusMs, int perfectRadiusMs);
DefenseCueGrade GradeCue(char expectedKey, char pressedKey, int timingErrorMs,
	int blockRadiusMs, int perfectRadiusMs);
DefenseResult ResolveSequence(const std::vector<DefenseCueGrade>& grades);
int DamageAfterDefense(int incomingDamage, DefenseResult result);

int CueCount(const TurnAction& action, const Spell* spell = nullptr);
DefenseChallenge BuildChallenge(const TurnAction& action, const Spell* spell,
	int enemySpeed, int enemyAttack, const std::vector<char>& keys);
std::string DescribeChallenge(const TurnAction& action, const Spell* spell,
	int enemySpeed, int enemyAttack);

} // namespace DefenseRules
