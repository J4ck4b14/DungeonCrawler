#pragma once
#include <string>
#include <array>
#include <vector>

// Cardinal directions
enum class Direction { North = 0, East = 1, South = 2, West = 3 };

inline const char* DirectionName(Direction d) {
	switch (d) {
	case Direction::North: return "North";
	case Direction::East:  return "East";
	case Direction::South: return "South";
	case Direction::West:  return "West";
	}
	return "???";
}

inline Direction OppositeDirection(Direction d) {
	switch (d) {
	case Direction::North: return Direction::South;
	case Direction::South: return Direction::North;
	case Direction::East:  return Direction::West;
	case Direction::West:  return Direction::East;
	}
	return d;
}

// What kind of content does a room have?
enum class RoomContent {
	Empty,
	Combat,
	Chest,
	Rest,
	Trap,
	Staircase   // Exit to next floor
};

// Perception hint about an adjacent room (generated once, then locked in)
struct PerceptionHint {
	Direction direction;
	std::string description;
	RoomContent revealedContent = RoomContent::Empty;  // What was revealed (canonical)
	bool revealsContent = false;  // Did the roll reveal the actual content?
};

struct Room {
	int x = 0;
	int y = 0;

	RoomContent content = RoomContent::Empty;
	bool visited = false;
	bool contentResolved = false;  // Has the encounter/chest/trap been dealt with?
	bool perceptionUsed = false;   // Can only perceive once per room

	// Which exits exist (N, E, S, W) - true means there's a passage
	std::array<bool, 4> exits = {false, false, false, false};

	bool HasExit(Direction d) const { return exits[static_cast<int>(d)]; }
	void SetExit(Direction d, bool open) { exits[static_cast<int>(d)] = open; }

	// Perception hints that have been given about adjacent rooms FROM this room
	std::vector<PerceptionHint> hints;
};
