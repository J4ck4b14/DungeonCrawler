#pragma once
#include "Encounter.h"
#include "entities/Player.h"

class Dungeon {
public:
	Dungeon();

	// Run a full dungeon floor. Returns true if player survives.
	bool RunFloor(Player& player);

	int GetCurrentLevel() const;

private:
	int currentLevel_;

	Encounter GenerateEncounter();
	void HandleEncounter(Player& player, const Encounter& encounter);
	void HandleChest(Player& player);
	void HandleRest(Player& player);
	void HandleTrap(Player& player);
};
