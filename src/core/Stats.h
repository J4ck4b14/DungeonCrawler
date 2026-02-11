// Stats.h
// -------
// Core stat block shared by all entities (players and enemies).
// Contains both raw allocation points (hp, atk, speed, intelligence)
// and derived values (maxHp, maxMana) computed by RecalculateDerived().
//
// Stat formulas:
//   maxHp   = 20 + hp * 5      (base 20 HP, +5 per point)
//   maxMana = intelligence * 3  (3 mana per INT point)
//
// ATK and Speed are set directly during character creation / enemy generation.

#pragma once
#include <string>
#include <iostream>

struct Stats {
	int hp = 0;       // Raw points allocated
	int maxHp = 0;    // Derived
	int atk = 0;
	int speed = 0;
	int intelligence = 0;
	int mana = 0;
	int maxMana = 0;

	void RecalculateDerived() {
		maxHp = 20 + hp * 5;          // Base 20 HP + 5 per point
		maxMana = intelligence * 3;   // 3 mana per intelligence point
	}

	void Print(const std::string& label) const {
		std::cout << label << " Stats - HP: " << maxHp
			<< " | ATK: " << atk
			<< " | SPD: " << speed
			<< " | INT: " << intelligence
			<< " | Mana: " << maxMana << "\n";
	}
};
