#pragma once

#include "Room.h"

class RNG;

struct RoomContentWeights {
	int combat;
	int chest;
	int trap;
	int rest;
	int empty;
};

namespace DungeonRules {

RoomContentWeights CalculateRoomContentWeights(
	int dungeonLevel, float enemyScale, float trapMultiplier);
int TotalRoomContentWeight(const RoomContentWeights& weights);
RoomContent SelectRoomContent(const RoomContentWeights& weights, int roll);
RoomContent RollRoomContent(const RoomContentWeights& weights, RNG& rng);

// Ordinary movement, room resolution, and map coverage never award XP.
constexpr int ExplorationXP() { return 0; }

} // namespace DungeonRules
