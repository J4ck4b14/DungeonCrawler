#pragma once
#include <string>
#include <iostream>

enum class ItemType {
	HealthPotion,
	ManaPotion
};

struct Item {
	std::string name;
	ItemType type;
	int potency; // Amount healed/restored

	void Describe() const {
		std::cout << name << " (restores " << potency;
		if (type == ItemType::HealthPotion) std::cout << " HP";
		else std::cout << " Mana";
		std::cout << ")\n";
	}
};

// Predefined items
inline Item MakeHealthPotion() { return {"Health Potion", ItemType::HealthPotion, 25}; }
inline Item MakeLargeHealthPotion() { return {"Large Health Potion", ItemType::HealthPotion, 50}; }
inline Item MakeManaPotion() { return {"Mana Potion", ItemType::ManaPotion, 15}; }
inline Item MakeLargeManaPotion() { return {"Large Mana Potion", ItemType::ManaPotion, 30}; }
