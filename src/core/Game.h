#pragma once
#include "entities/Player.h"
#include "dungeon/Dungeon.h"

class Game {
public:
	Game() = default;
	void Run();

private:
	void ShowTitle() const;
};
