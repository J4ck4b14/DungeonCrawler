#pragma once
#include <string>

enum class EncounterType {
	Combat,
	Chest,
	Rest,       // Small HP/Mana regen
	Trap,       // Take some damage
	Nothing     // Empty room
};

struct Encounter {
	EncounterType type;
	std::string description;
};
