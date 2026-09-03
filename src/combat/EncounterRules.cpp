#include "EncounterRules.h"

#include <algorithm>

namespace EncounterRules {

int MultiEnemyChancePercent(int dungeonLevel) {
	if (dungeonLevel < 2) return 0;
	return std::clamp(8 + (dungeonLevel - 2) * 2, 8, 28);
}

int ThreeEnemyChancePercent(int dungeonLevel) {
	if (dungeonLevel < 6) return 0;
	return std::clamp(2 + (dungeonLevel - 6), 2, 6);
}

int DetermineEnemyCount(int dungeonLevel, int percentileRoll) {
	percentileRoll = std::clamp(percentileRoll, 1, 100);
	if (percentileRoll <= ThreeEnemyChancePercent(dungeonLevel)) return 3;
	if (percentileRoll <= MultiEnemyChancePercent(dungeonLevel)) return 2;
	return 1;
}

} // namespace EncounterRules
