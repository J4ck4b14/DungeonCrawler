#pragma once
#include <string>
#include <array>
#include <vector>
#include "combat/Spell.h" // for SpellElement

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

// What the player remembers happening in a resolved room. This is deliberately
// separate from RoomContent: a trap can be triggered or safely disarmed, and
// perception text should describe the result rather than the original hazard.
enum class RoomOutcome {
	Unresolved,
	EnemyDefeated,
	ChestOpened,
	Rested,
	TrapTriggered,
	TrapDisarmed,
	EmptySearched
};

// Perception hint about an adjacent room (generated once, then locked in)
struct PerceptionHint {
	Direction direction;
	std::string description;
	RoomContent revealedContent = RoomContent::Empty;  // What was revealed (canonical)
	bool revealsContent = false;  // Did the roll reveal the actual content?
};

// Basic wall/material model for hidden/brittle walls
enum class WallMaterial {
	None = 0,
	Wood,
	Stone,
	Strange   // exotic deep-materia (often vulnerable only to specific magic)
};

// Geometry and player knowledge are deliberately separate. A wall may border
// generated space without the player knowing that space exists.
enum class WallType {
	Solid,
	AdjacentUnconnected,
	Concealed
};

enum class WallKnowledge {
	Unknown,
	Suspected,
	Revealed
};

struct HiddenWall {
	bool exists = false;
	bool breakable = false;
	WallType type = WallType::Solid;
	WallKnowledge knowledge = WallKnowledge::Unknown;
	int toughness = 0;
	WallMaterial material = WallMaterial::None;
	SpellElement weakness = SpellElement::Arcane;
};

struct Room {
	int x = 0;
	int y = 0;
	bool exists = false;
	bool mapRevealed = false;

	RoomContent content = RoomContent::Empty;
	bool visited = false;
	bool contentResolved = false;  // Has the encounter/chest/trap been dealt with?
	RoomOutcome outcome = RoomOutcome::Unresolved;
	bool perceptionUsed = false;   // Can only perceive once per room

	// Which exits exist (N, E, S, W) - true means there's a passage
	std::array<bool, 4> exits = {false, false, false, false};

	bool HasExit(Direction d) const { return exits[static_cast<int>(d)]; }
	void SetExit(Direction d, bool open) { exits[static_cast<int>(d)] = open; }

	// Hidden/breakable exits (not a normal passage until broken)
	std::array<HiddenWall, 4> hiddenWalls{};

	const HiddenWall& GetHiddenWall(Direction d) const {
		return hiddenWalls[static_cast<int>(d)];
	}
	bool HasHiddenExit(Direction d) const { return GetHiddenWall(d).exists; }
	int GetHiddenToughness(Direction d) const { return GetHiddenWall(d).toughness; }
	void SetHiddenWall(Direction d, int toughness, WallMaterial material,
		SpellElement weakness, WallType type = WallType::Concealed,
		bool breakable = true) {
		hiddenWalls[static_cast<int>(d)] = {
			true, breakable, type, WallKnowledge::Unknown,
			toughness, material, weakness};
	}
	bool IsWallBreakable(Direction d) const {
		const HiddenWall& wall = GetHiddenWall(d);
		return wall.exists && wall.breakable && wall.toughness > 0;
	}
	bool IsWallSuspected(Direction d) const {
		return GetHiddenWall(d).knowledge == WallKnowledge::Suspected;
	}
	void SetWallKnowledge(Direction d, WallKnowledge knowledge) {
		hiddenWalls[static_cast<int>(d)].knowledge = knowledge;
	}
	void SetHiddenToughness(Direction d, int toughness) {
		hiddenWalls[static_cast<int>(d)].toughness = toughness;
	}
	void ClearHiddenWall(Direction d) {
		hiddenWalls[static_cast<int>(d)] = {};
	}

	// Perception hints that have been given about adjacent rooms FROM this room
	std::vector<PerceptionHint> hints;
};
