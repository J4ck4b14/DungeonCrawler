#pragma once

class Player;
class Enemy;

class CombatSystem {
public:
	// Returns true if player wins, false if player dies
	static bool ResolveCombat(Player& player, Enemy& enemy);
};
