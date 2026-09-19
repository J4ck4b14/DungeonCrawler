#include "DungeonTopology.h"

#include "utils/RNG.h"

#include <algorithm>
#include <array>
#include <queue>
#include <sstream>
#include <utility>

namespace {
constexpr std::array<int, 4> DX{0, 1, 0, -1};
constexpr std::array<int, 4> DY{-1, 0, 1, 0};

bool InBounds(int x, int y, int size) {
	return x >= 0 && y >= 0 && x < size && y < size;
}

void Connect(std::vector<std::vector<Room>>& grid, int x, int y, Direction direction) {
	const int d = static_cast<int>(direction);
	const int nx = x + DX[d], ny = y + DY[d];
	grid[y][x].SetExit(direction, true);
	grid[ny][nx].SetExit(OppositeDirection(direction), true);
}

std::vector<int> Distances(const GeneratedFloorTopology& floor) {
	std::vector<int> distances(floor.size * floor.size, -1);
	std::queue<std::pair<int, int>> pending;
	distances[floor.entranceY * floor.size + floor.entranceX] = 0;
	pending.push({floor.entranceX, floor.entranceY});
	while (!pending.empty()) {
		auto [x, y] = pending.front(); pending.pop();
		for (int d = 0; d < 4; ++d) {
			if (!floor.grid[y][x].exits[d]) continue;
			const int nx = x + DX[d], ny = y + DY[d];
			const int index = ny * floor.size + nx;
			if (distances[index] >= 0) continue;
			distances[index] = distances[y * floor.size + x] + 1;
			pending.push({nx, ny});
		}
	}
	return distances;
}

char Marker(const Room& room, bool player) {
	if (player) return '@';
	if (room.content == RoomContent::Staircase) return 'V';
	if (!room.visited) return 'o';
	switch (room.outcome) {
	case RoomOutcome::EnemyDefeated: return 'x';
	case RoomOutcome::ChestOpened: return 'c';
	case RoomOutcome::Rested: return 'r';
	case RoomOutcome::TrapTriggered: return '!';
	case RoomOutcome::TrapDisarmed: return '^';
	case RoomOutcome::EmptySearched: return '.';
	default: return ' ';
	}
}
}

GeneratedFloorTopology DungeonTopology::Generate(int dungeonLevel, RNG& rng) {
	GeneratedFloorTopology floor;
	floor.size = std::min(8, 5 + std::max(0, dungeonLevel - 1) / 4);
	floor.grid.assign(floor.size, std::vector<Room>(floor.size));
	for (int y = 0; y < floor.size; ++y) for (int x = 0; x < floor.size; ++x) {
		floor.grid[y][x].x = x; floor.grid[y][x].y = y;
	}

	const int edge = rng.NextInt(0, 3);
	if (edge == 0) { floor.entranceX = rng.NextInt(0, floor.size - 1); floor.entranceY = 0; }
	if (edge == 1) { floor.entranceX = floor.size - 1; floor.entranceY = rng.NextInt(0, floor.size - 1); }
	if (edge == 2) { floor.entranceX = rng.NextInt(0, floor.size - 1); floor.entranceY = floor.size - 1; }
	if (edge == 3) { floor.entranceX = 0; floor.entranceY = rng.NextInt(0, floor.size - 1); }

	const int target = std::max(12, floor.size * floor.size * rng.NextInt(55, 72) / 100);
	floor.grid[floor.entranceY][floor.entranceX].exists = true;
	std::vector<std::pair<int, int>> rooms{{floor.entranceX, floor.entranceY}};
	while (static_cast<int>(rooms.size()) < target) {
		std::vector<std::array<int, 3>> candidates;
		for (const auto& [x, y] : rooms) {
			for (int d = 0; d < 4; ++d) {
				const int nx = x + DX[d], ny = y + DY[d];
				if (InBounds(nx, ny, floor.size) && !floor.grid[ny][nx].exists)
					candidates.push_back({x, y, d});
			}
		}
		if (candidates.empty()) break;
		// A bias toward recent growth creates both long arms and branching hubs.
		const int index = rng.Chance(0.62f)
			? rng.NextInt(std::max(0, static_cast<int>(candidates.size()) - 8), static_cast<int>(candidates.size()) - 1)
			: rng.NextInt(0, static_cast<int>(candidates.size()) - 1);
		const auto chosen = candidates[index];
		const int nx = chosen[0] + DX[chosen[2]], ny = chosen[1] + DY[chosen[2]];
		floor.grid[ny][nx].exists = true;
		Connect(floor.grid, chosen[0], chosen[1], static_cast<Direction>(chosen[2]));
		rooms.push_back({nx, ny});
	}

	auto distances = Distances(floor);
	int farthest = -1;
	for (const auto& [x, y] : rooms) {
		const int distance = distances[y * floor.size + x];
		if (distance > farthest) {
			farthest = distance; floor.stairsX = x; floor.stairsY = y;
		}
	}

	// Selective loops keep the skeleton legible and allow distant reconnections.
	std::vector<std::array<int, 3>> possibleLoops;
	for (int y = 0; y < floor.size; ++y) for (int x = 0; x < floor.size; ++x) {
		if (!floor.grid[y][x].exists) continue;
		for (int d : {1, 2}) {
			const int nx = x + DX[d], ny = y + DY[d];
			if (InBounds(nx, ny, floor.size) && floor.grid[ny][nx].exists
				&& !floor.grid[y][x].exits[d]) possibleLoops.push_back({x, y, d});
		}
	}
	const int desiredLoops = rng.Chance(0.22f) ? 0 : rng.NextInt(1, std::max(1, target / 10));
	for (int i = 0; i < desiredLoops && !possibleLoops.empty(); ++i) {
		const int index = rng.NextInt(0, static_cast<int>(possibleLoops.size()) - 1);
		const auto edgeChoice = possibleLoops[index];
		Connect(floor.grid, edgeChoice[0], edgeChoice[1], static_cast<Direction>(edgeChoice[2]));
		possibleLoops.erase(possibleLoops.begin() + index);
	}

	// Every remaining physical adjacency has explicit generator truth, but only a
	// minority is a useful breakable route and none is initially player-visible.
	for (int y = 0; y < floor.size; ++y) for (int x = 0; x < floor.size; ++x) {
		if (!floor.grid[y][x].exists) continue;
		for (int d : {1, 2}) {
			const int nx = x + DX[d], ny = y + DY[d];
			if (!InBounds(nx, ny, floor.size) || !floor.grid[ny][nx].exists
				|| floor.grid[y][x].exits[d]) continue;
			const WallType type = rng.Chance(0.28f) ? WallType::Concealed : WallType::AdjacentUnconnected;
			const bool breakable = type == WallType::Concealed || rng.Chance(0.55f);
			int toughness = breakable ? rng.NextInt(7, 15) + dungeonLevel / 2 : 0;
			WallMaterial material = toughness <= 9 ? WallMaterial::Wood : WallMaterial::Stone;
			SpellElement weakness = material == WallMaterial::Wood ? SpellElement::Fire : SpellElement::Arcane;
			floor.grid[y][x].SetHiddenWall(static_cast<Direction>(d), toughness,
				material, weakness, type, breakable);
			floor.grid[ny][nx].SetHiddenWall(OppositeDirection(static_cast<Direction>(d)),
				toughness, material, weakness, type, breakable);
		}
	}
	floor.metrics = Measure(floor);
	return floor;
}

TopologyMetrics DungeonTopology::Measure(const GeneratedFloorTopology& floor) {
	TopologyMetrics metrics;
	for (const auto& row : floor.grid) for (const Room& room : row) {
		if (!room.exists) continue;
		++metrics.roomCount;
		const int degree = static_cast<int>(std::count(room.exits.begin(), room.exits.end(), true));
		metrics.passageCount += degree;
		if (degree == 1) ++metrics.deadEndCount;
		if (degree >= 3) ++metrics.junctionCount;
	}
	metrics.passageCount /= 2;
	metrics.loopCount = std::max(0, metrics.passageCount - metrics.roomCount + 1);
	const auto distances = Distances(floor);
	metrics.shortestEntranceToStairs = distances[floor.stairsY * floor.size + floor.stairsX];
	return metrics;
}

bool DungeonTopology::MandatorySpacesConnected(const GeneratedFloorTopology& floor) {
	const auto distances = Distances(floor);
	for (int y = 0; y < floor.size; ++y) for (int x = 0; x < floor.size; ++x) {
		if (floor.grid[y][x].exists && distances[y * floor.size + x] < 0) return false;
	}
	return distances[floor.stairsY * floor.size + floor.stairsX] >= 0;
}

bool DungeonTopology::OpenWall(GeneratedFloorTopology& floor, int x, int y, Direction direction) {
	const int d = static_cast<int>(direction), nx = x + DX[d], ny = y + DY[d];
	if (!InBounds(x, y, floor.size) || !InBounds(nx, ny, floor.size)) return false;
	Room& from = floor.grid[y][x]; Room& to = floor.grid[ny][nx];
	if (!from.exists || !to.exists || !from.IsWallBreakable(direction)) return false;
	from.ClearHiddenWall(direction); to.ClearHiddenWall(OppositeDirection(direction));
	from.SetExit(direction, true); to.SetExit(OppositeDirection(direction), true);
	to.mapRevealed = true;
	return true;
}

std::string DungeonTopology::RenderKnowledge(const GeneratedFloorTopology& floor,
	int playerX, int playerY, bool debugReveal) {
	std::ostringstream out;
	for (int y = 0; y < floor.size; ++y) {
		for (int x = 0; x < floor.size; ++x) {
			const Room& room = floor.grid[y][x];
			const bool known = debugReveal ? room.exists : (room.visited || room.mapRevealed);
			char marker = Marker(room, x == playerX && y == playerY);
			if (x != playerX || y != playerY) {
				if (x == floor.stairsX && y == floor.stairsY) marker = 'V';
			}
			out << (known ? marker : ' ');
			if (x + 1 < floor.size) {
				const Room& east = floor.grid[y][x + 1];
				const bool eastKnown = debugReveal ? east.exists : (east.visited || east.mapRevealed);
				out << (known && eastKnown && room.HasExit(Direction::East) ? '-' : ' ');
			}
		}
		out << '\n';
		if (y + 1 < floor.size) {
			for (int x = 0; x < floor.size; ++x) {
				const Room& room = floor.grid[y][x];
				const Room& south = floor.grid[y + 1][x];
				const bool known = debugReveal ? room.exists : (room.visited || room.mapRevealed);
				const bool southKnown = debugReveal ? south.exists : (south.visited || south.mapRevealed);
				out << (known && southKnown && room.HasExit(Direction::South) ? '|' : ' ');
				if (x + 1 < floor.size) out << ' ';
			}
			out << '\n';
		}
	}
	if (!debugReveal && InBounds(playerX, playerY, floor.size)) {
		const Room& current = floor.grid[playerY][playerX];
		bool heading = false;
		for (int d = 0; d < 4; ++d) if (current.IsWallSuspected(static_cast<Direction>(d))) {
			if (!heading) { out << "Suspicious walls:"; heading = true; }
			out << ' ' << DirectionName(static_cast<Direction>(d));
		}
		if (heading) out << '\n';
	}
	return out.str();
}
