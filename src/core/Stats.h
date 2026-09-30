// Compact stat block shared by generated enemies and the player. `hp` is the
// input used to derive max HP; strength/speed/intelligence are combat values by
// the time an Entity is constructed. RecalculateDerived also derives max mana.

#pragma once
#include <string>
#include <iostream>

struct Stats {
	int hp = 0;       // HP input used by RecalculateDerived
	int maxHp = 0;
	int strength = 0;
	int speed = 0;
	int intelligence = 0;
	int maxMana = 0;

	void RecalculateDerived() {
		maxHp = 20 + hp * 5;          // Base 20 HP + 5 per point
		maxMana = intelligence * 3;   // 3 mana per intelligence point
	}

	void Print(const std::string& label) const {
		std::cout << label << " Stats - HP: " << maxHp
			<< " | STR: " << strength
			<< " | SPD: " << speed
			<< " | INT: " << intelligence
			<< " | Mana: " << maxMana << "\n";
	}
};
