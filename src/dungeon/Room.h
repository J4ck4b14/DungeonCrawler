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

	// Hidden/breakable exits (not a normal passage until broken)
	std::array<bool, 4> hiddenExits = {false, false, false, false};
	// Hidden wall "HP" (internal, unknown to player)
	std::array<int, 4> hiddenToughness = {0, 0, 0, 0};
	// Material and hidden elemental weakness for the wall
	std::array<WallMaterial, 4> hiddenMaterial = {
		WallMaterial::None, WallMaterial::None, WallMaterial::None, WallMaterial::None
	};
	std::array<SpellElement, 4> hiddenWeakness = {
		SpellElement::Arcane, SpellElement::Arcane, SpellElement::Arcane, SpellElement::Arcane
	};

	bool HasHiddenExit(Direction d) const { return hiddenExits[static_cast<int>(d)]; }
	int GetHiddenToughness(Direction d) const { return hiddenToughness[static_cast<int>(d)]; }
	void SetHiddenExit(Direction d, bool hasHidden, int toughness = 0) {
		hiddenExits[static_cast<int>(d)] = hasHidden;
		hiddenToughness[static_cast<int>(d)] = hasHidden ? toughness : 0;
		if (!hasHidden) {
			hiddenMaterial[static_cast<int>(d)] = WallMaterial::None;
			hiddenWeakness[static_cast<int>(d)] = SpellElement::Arcane;
		}
	}

	// Perception hints that have been given about adjacent rooms FROM this room
	std::vector<PerceptionHint> hints;
};
