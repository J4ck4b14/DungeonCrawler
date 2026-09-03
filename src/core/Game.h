#pragma once
#include "entities/Player.h"
#include "dungeon/Dungeon.h"

class PlayerProfile;

class Game {
public:
	Game() = default;
	void Run();

private:
	void ShowTitle() const;
	void RunAdventure(PlayerProfile& profile);
};
