#pragma once

struct RoomContentWeights {
	int combat;
	int chest;
	int trap;
	int rest;
};

namespace DungeonRules {

RoomContentWeights CalculateRoomContentWeights(
	int dungeonLevel, float enemyScale, float trapMultiplier);

} // namespace DungeonRules
