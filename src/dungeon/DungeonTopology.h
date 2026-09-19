#pragma once

#include "Room.h"

#include <string>
#include <vector>

class RNG;

struct TopologyMetrics {
	int roomCount = 0;
	int passageCount = 0;
	int deadEndCount = 0;
	int junctionCount = 0;
	int loopCount = 0;
	int shortestEntranceToStairs = -1;
};

struct GeneratedFloorTopology {
	std::vector<std::vector<Room>> grid;
	int size = 0;
	int entranceX = 0;
	int entranceY = 0;
	int stairsX = 0;
	int stairsY = 0;
	TopologyMetrics metrics;
};

namespace DungeonTopology {
	GeneratedFloorTopology Generate(int dungeonLevel, RNG& rng);
	TopologyMetrics Measure(const GeneratedFloorTopology& floor);
	bool MandatorySpacesConnected(const GeneratedFloorTopology& floor);
	bool OpenWall(GeneratedFloorTopology& floor, int x, int y, Direction direction);
	std::string RenderKnowledge(const GeneratedFloorTopology& floor,
		int playerX, int playerY, bool debugReveal = false);
}
