#pragma once
#include "Room.h"

class Player;

class Perception {
public:
	// Roll the hidden base d20. The survey applies the player's INT modifier.
	static int Roll();

	// Survey every direction once. All resulting text is remembered, but only
	// sufficiently strong truthful hints mark adjacent content as actually known.
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
