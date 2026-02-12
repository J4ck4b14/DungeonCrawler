#pragma once
#include "Room.h"

class Player;

class Perception {
public:
	// Roll a d20 + INT modifier. Returns the roll result (1-20 base).
	static int Roll(const Player& player);

	// Generate a description based on the roll for what the player senses
	// about adjacent rooms. This creates PerceptionHints that become canonical.
	static void PerceiveFromRoom(Room& currentRoom, 
		const std::vector<std::vector<Room>>& grid,
		int gridSize, const Player& player);

private:
	// Describe a direction based on how much was revealed
	static std::string DescribeDirection(Direction dir, const Room& adjacent, 
		int rollQuality);
	
	// Describe a wall (no exit in that direction). The hasHidden flag
	// indicates a brittle/breakable wall; toughness gives its break target.
	static std::string DescribeWall(Direction dir, bool hasHidden = false, int toughness = 0);
};
