#pragma once

namespace EncounterRules {

int MultiEnemyChancePercent(int dungeonLevel);
int ThreeEnemyChancePercent(int dungeonLevel);
int DetermineEnemyCount(int dungeonLevel, int percentileRoll);

} // namespace EncounterRules
