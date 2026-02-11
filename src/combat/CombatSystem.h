// CombatSystem.h
// ---------------
// Static class that resolves turn-based combat between a Player and an Enemy.
// Tracks enemy knowledge (None/Approximate/Partial/Full) and game statistics.
// The Bestiary provides persistent cross-floor enemy knowledge.

#pragma once
#include <string>
#include <set>

class Player;
class Enemy;
struct GameStats;
class Bestiary;

class CombatSystem {
public:
	// Returns true if player wins, false if player dies
	static bool ResolveCombat(Player& player, Enemy& enemy,
		std::set<std::string>& seenEnemyTypes, GameStats& stats,
		Bestiary& bestiary);
};
