#include "DungeonRules.h"
#include "utils/RNG.h"

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
	weights.empty = std::max(4, 23 - std::max(0, dungeonLevel - 1) / 2);
	return weights;
}

int DungeonRules::TotalRoomContentWeight(const RoomContentWeights& weights) {
	return std::max(0, weights.combat) + std::max(0, weights.chest)
		+ std::max(0, weights.trap) + std::max(0, weights.rest)
		+ std::max(0, weights.empty);
}

RoomContent DungeonRules::SelectRoomContent(const RoomContentWeights& weights, int roll) {
	const int total = TotalRoomContentWeight(weights);
	if (total <= 0) return RoomContent::Empty;
	roll = std::clamp(roll, 1, total);
	roll -= std::max(0, weights.combat);
	if (roll <= 0) return RoomContent::Combat;
	roll -= std::max(0, weights.chest);
	if (roll <= 0) return RoomContent::Chest;
	roll -= std::max(0, weights.trap);
	if (roll <= 0) return RoomContent::Trap;
	roll -= std::max(0, weights.rest);
	if (roll <= 0) return RoomContent::Rest;
	return RoomContent::Empty;
}

RoomContent DungeonRules::RollRoomContent(const RoomContentWeights& weights, RNG& rng) {
	const int total = TotalRoomContentWeight(weights);
	return total > 0 ? SelectRoomContent(weights, rng.NextInt(1, total))
		: RoomContent::Empty;
}
