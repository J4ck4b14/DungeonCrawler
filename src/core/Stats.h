#pragma once
#include <string>
#include <iostream>

struct Stats {
	int hp = 0;
	int maxHp = 0;
	int atk = 0;
	int speed = 0;
	int intelligence = 0;
	int mana = 0;
	int maxMana = 0;

	void RecalculateDerived() {
		maxHp = 50 + hp * 10;       // Base 50 HP + 10 per point
		maxMana = intelligence * 5;  // 5 mana per intelligence point
	}

	void Print(const std::string& label) const {
		std::cout << label << " Stats — HP: " << maxHp
			<< " | ATK: " << atk
			<< " | SPD: " << speed
			<< " | INT: " << intelligence
			<< " | Mana: " << maxMana << "\n";
	}
};
