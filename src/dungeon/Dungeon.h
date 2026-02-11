#pragma once
#include "Room.h"
#include "Encounter.h"
#include "entities/Player.h"
#include "core/GameStats.h"
#include <vector>
#include <set>

class Dungeon {
public:
	Dungeon();

	// Run a full dungeon floor. Returns true if player survives and finds the exit.
	bool RunFloor(Player& player);

	int GetCurrentLevel() const;
	const GameStats& GetStats() const;

private:
	int currentLevel_;

	// Grid for current floor
	std::vector<std::vector<Room>> grid_;
	int gridSize_;
	int playerX_, playerY_;

	// Enemy types seen on this floor (for approximate stats)
	std::set<std::string> seenEnemyTypes_;

	// Persistent stats across the whole run
	GameStats gameStats_;

	// Floor generation
	void GenerateFloor();
	RoomContent GenerateRoomContent(bool isStart, bool isStaircase);

	// Room handling
	void EnterRoom(Player& player);
	void HandleRoomContent(Player& player, Room& room);
	void HandleCombat(Player& player);
	void HandleChest(Player& player);
	void HandleRest(Player& player);
	void HandleTrap(Player& player, Room& room);

	// Movement
	bool PromptMovement(Player& player);

	// Map display
	void PrintMap() const;

	// XP for exploration
	void AwardExplorationXP(Player& player);
};
