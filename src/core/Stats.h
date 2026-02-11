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
