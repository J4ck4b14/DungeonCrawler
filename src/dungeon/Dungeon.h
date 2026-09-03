#pragma once
#include "Room.h"
#include "entities/Player.h"
#include "core/GameStats.h"
#include "core/Bestiary.h"
#include <vector>
#include <set>

enum class FloorResult {
	Cleared,
	PlayerDied
};

class PlayerProfile;

class Dungeon {
public:
	explicit Dungeon(const PlayerProfile* profile = nullptr);

	FloorResult RunFloor(Player& player);

	int GetCurrentLevel() const;
	const GameStats& GetStats() const;
	const Bestiary& GetBestiary() const;

private:
	enum class MovementResult {
		ReachedStairs,
		PlayerDied
	};

	int currentLevel_;

	// Grid for current floor
	std::vector<std::vector<Room>> grid_;
	int gridSize_;
	int playerX_, playerY_;

	// Enemy types seen on this floor (for approximate stats)
	std::set<std::string> seenEnemyTypes_;

	// Persistent stats across the whole run
	GameStats gameStats_;
	Bestiary bestiary_;
	const PlayerProfile* profile_ = nullptr;

	// Floor generation
	void GenerateFloor();
	RoomContent GenerateRoomContent(bool isStart, bool isStaircase);

	// Room handling
	void EnterRoom(Player& player);
	void HandleRoomContent(Player& player, Room& room);
	bool HandleCombat(Player& player);
	void HandleChest(Player& player);
	void HandleRest(Player& player);
	RoomOutcome HandleTrap(Player& player, Room& room);

	// Movement
	MovementResult PromptMovement(Player& player);

	// Map display
	void PrintMap() const;

	// XP for exploration
	void AwardExplorationXP(Player& player);
};
