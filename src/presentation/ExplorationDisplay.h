#pragma once

class Player;
struct Room;

namespace ExplorationDisplay {

void PrintStatusPanel(int floor, int playerX, int playerY,
	int visitedRooms, int totalRooms, const Room& currentRoom,
	const Player& player);

} // namespace ExplorationDisplay
