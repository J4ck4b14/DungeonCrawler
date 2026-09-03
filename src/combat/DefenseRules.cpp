#include "DefenseRules.h"

#include "Spell.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace {

char NormalizeKey(char key) {
	return static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
}

std::string SpeedName(int fallDurationMs) {
	if (fallDurationMs <= 750) return "very fast";
	if (fallDurationMs <= 950) return "fast";
	if (fallDurationMs <= 1150) return "measured";
	return "slow";
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

int CueCount(const TurnAction& action, const Spell* spell) {
	if (action.type == ActionType::CastSpell) {
		return std::clamp(2 + (spell ? spell->manaCost / 3 : 0), 2, 4);
	}
	if (action.type != ActionType::Attack) return 1;
	switch (action.attackStyle) {
	case AttackStyle::Slash: return 2;
	case AttackStyle::Thrust: return 1;
	case AttackStyle::Bash: return 2;
	}
	return 1;
}

DefenseChallenge BuildChallenge(const TurnAction& action, const Spell* spell,
	int enemySpeed, int enemyAttack, const std::vector<char>& keys) {
	DefenseChallenge challenge;
	const int speed = std::max(1, enemySpeed);
	const int powerPressure = std::clamp(std::max(0, enemyAttack) / 5, 0, 8);
	const int difficulty = speed + powerPressure;
	int baseFall = std::clamp(1480 - difficulty * 60, 560, 1400);
	int gap = std::clamp(650 - difficulty * 28, 210, 620);
	challenge.blockRadiusMs = std::clamp(285 - difficulty * 10, 125, 260);
	challenge.perfectRadiusMs = std::clamp(105 - difficulty * 5, 42, 90);

	if (action.type == ActionType::CastSpell) {
		challenge.attackLabel = spell ? spell->name : "Hostile spell";
		baseFall = std::max(560, baseFall - 80);
	}
	else if (action.type == ActionType::Attack) {
		switch (action.attackStyle) {
		case AttackStyle::Slash:
			challenge.attackLabel = "Sweeping slash";
			break;
		case AttackStyle::Thrust:
			challenge.attackLabel = "Driving thrust";
			baseFall = std::max(520, baseFall - 180);
			break;
		case AttackStyle::Bash:
			challenge.attackLabel = "Crushing bash";
			baseFall = std::min(1500, baseFall + 130);
			gap += 100;
			break;
		}
	}
	else {
		challenge.attackLabel = "Incoming attack";
	}

	const int count = CueCount(action, spell);
	challenge.cues.reserve(static_cast<size_t>(count));
	for (int i = 0; i < count; ++i) {
		DefenseCue cue;
		cue.key = i < static_cast<int>(keys.size()) ? NormalizeKey(keys[i]) : 'W';
		cue.fallDurationMs = baseFall;
		cue.gapAfterMs = gap;
		if (action.type == ActionType::Attack && action.attackStyle == AttackStyle::Bash
			&& i == count - 1) {
			cue.fallDurationMs = std::max(520, baseFall - 220);
		}
		challenge.cues.push_back(cue);
	}
	return challenge;
}

std::string DescribeChallenge(const TurnAction& action, const Spell* spell,
	int enemySpeed, int enemyAttack) {
	const DefenseChallenge challenge = BuildChallenge(
		action, spell, enemySpeed, enemyAttack, {});
	const int count = static_cast<int>(challenge.cues.size());
	const int fall = challenge.cues.empty() ? 1000 : challenge.cues.front().fallDurationMs;
	return std::to_string(count) + (count == 1 ? " cue, " : " cues, ")
		+ SpeedName(fall);
}

} // namespace DefenseRules
