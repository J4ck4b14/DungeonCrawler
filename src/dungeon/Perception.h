#pragma once
#include "Room.h"

class Player;

class Perception {
public:
	// Roll the base d20. The caller applies the player's INT modifier.
	static int Roll();

	// Generate a description based on the roll for what the player senses
	// about adjacent rooms. This creates PerceptionHints that become canonical.
	static void PerceiveFromRoom(Room& currentRoom, 
		const std::vector<std::vector<Room>>& grid,
		int gridSize, const Player& player);

	// A concealed space may sound unusual without implying its wall can break.
	static std::string DescribeWall(Direction dir, bool unusual = false,
		bool breakable = false, int toughness = 0, int quality = 0,
		int playerStrength = 0);

private:
	// Describe a direction based on how much was revealed
	static std::string DescribeDirection(Direction dir, const Room& adjacent, 
		int rollQuality);
	
};
