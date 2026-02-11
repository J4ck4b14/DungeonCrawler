#pragma once
#include <string>
#include <set>

class Player;
class Enemy;
struct GameStats;

class CombatSystem {
public:
	// Returns true if player wins, false if player dies
	static bool ResolveCombat(Player& player, Enemy& enemy,
		std::set<std::string>& seenEnemyTypes, GameStats& stats);
};
