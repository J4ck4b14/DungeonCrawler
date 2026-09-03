#include "DungeonRules.h"

#include <algorithm>

RoomContentWeights DungeonRules::CalculateRoomContentWeights(
	int dungeonLevel, float enemyScale, float trapMultiplier) {
	RoomContentWeights weights{};
	weights.combat = static_cast<int>((40 + dungeonLevel * 2) * enemyScale);
	weights.chest = static_cast<int>(15 * (1.0f / enemyScale));
	weights.trap = static_cast<int>((8 + dungeonLevel) * trapMultiplier);

	const int baseRestChance = 12 - dungeonLevel;
	weights.rest = trapMultiplier > 0.0f
		? static_cast<int>(baseRestChance * (1.0f / trapMultiplier))
		: (baseRestChance > 0 ? 100 : 2);
	weights.rest = std::max(2, weights.rest);
	return weights;
}
