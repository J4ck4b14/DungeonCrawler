// Item.h
// ------
// Defines consumable items (potions) that players can find in chests
// and use during combat or exploration.
//
// ItemType: HealthPotion (restores HP), ManaPotion (restores Mana).
// Each item has a name, type, and potency (amount restored).
//
// Factory functions create predefined items:
//   MakeHealthPotion()      -> 10 HP
//   MakeLargeHealthPotion() -> 20 HP
//   MakeManaPotion()        -> 6 Mana
//   MakeLargeManaPotion()   -> 12 Mana

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
inline Item MakeHealthPotion() { return {"Health Potion", ItemType::HealthPotion, 10}; }
inline Item MakeLargeHealthPotion() { return {"Large Health Potion", ItemType::HealthPotion, 20}; }
inline Item MakeManaPotion() { return {"Mana Potion", ItemType::ManaPotion, 6}; }
inline Item MakeLargeManaPotion() { return {"Large Mana Potion", ItemType::ManaPotion, 12}; }
