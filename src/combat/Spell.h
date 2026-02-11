// Spell.h
// -------
// Defines the spell system for DungeonCrawler.
//
// SpellElement: Fire, Ice, Lightning, Healing, Shadow, Arcane.
//   Each enemy has a weakness to one element (1.5x damage).
//
// SpellTarget: Enemy (damage) or Self (heal/buff).
//
// The master spell catalog (GetSpellCatalog) contains all spells in the game,
// organized by element and tier. Spells have an intelligence requirement
// to learn -- the player can acquire spells by defeating enemies that know them.
//
// FindSpell() looks up a spell by name from the catalog.

#pragma once
#include <string>
#include <vector>
#include <functional>

enum class SpellElement {
	Fire,
	Ice,
	Lightning,
	Healing,
	Shadow,
	Arcane
};

enum class SpellTarget {
	Enemy,   // Damage spell
	Self     // Buff/heal spell
};

struct Spell {
	std::string name;
	SpellElement element;
	SpellTarget target;
	int manaCost;
	int power;             // Damage or heal amount (base)
	int requiredIntelligence; // Minimum INT to learn/use

	std::string GetElementName() const {
		switch (element) {
		case SpellElement::Fire: return "Fire";
		case SpellElement::Ice: return "Ice";
		case SpellElement::Lightning: return "Lightning";
		case SpellElement::Healing: return "Healing";
		case SpellElement::Shadow: return "Shadow";
		case SpellElement::Arcane: return "Arcane";
		default: return "Unknown";
		}
	}
};

// -- Master Spell Catalog --
inline const std::vector<Spell>& GetSpellCatalog() {
	static const std::vector<Spell> catalog = {
		// -- Offensive: Fire --
		{"Fireball",       SpellElement::Fire,      SpellTarget::Enemy, 3,   6, 2},
		{"Inferno",        SpellElement::Fire,      SpellTarget::Enemy, 6,  12, 4},
		{"Flame Lance",    SpellElement::Fire,      SpellTarget::Enemy, 4,   8, 3},

		// -- Offensive: Ice --
		{"Frost Bolt",     SpellElement::Ice,       SpellTarget::Enemy, 2,   4, 1},
		{"Blizzard",       SpellElement::Ice,       SpellTarget::Enemy, 7,  14, 5},
		{"Ice Shard",      SpellElement::Ice,       SpellTarget::Enemy, 3,   6, 2},

		// -- Offensive: Lightning --
		{"Spark",          SpellElement::Lightning,  SpellTarget::Enemy, 1,   3, 1},
		{"Thunderbolt",    SpellElement::Lightning,  SpellTarget::Enemy, 5,  10, 3},
		{"Chain Lightning",SpellElement::Lightning,  SpellTarget::Enemy, 8,  16, 5},

		// -- Offensive: Shadow --
		{"Shadow Bolt",    SpellElement::Shadow,     SpellTarget::Enemy, 3,   5, 2},
		{"Void Blast",     SpellElement::Shadow,     SpellTarget::Enemy, 6,  12, 4},
		{"Soul Drain",     SpellElement::Shadow,     SpellTarget::Enemy, 4,   7, 3},

		// -- Offensive: Arcane --
		{"Magic Missile",  SpellElement::Arcane,     SpellTarget::Enemy, 1,   3, 1},
		{"Arcane Burst",   SpellElement::Arcane,     SpellTarget::Enemy, 5,  10, 3},

		// -- Healing --
		{"Heal",           SpellElement::Healing,    SpellTarget::Self,  2,   8, 1},
		{"Greater Heal",   SpellElement::Healing,    SpellTarget::Self,  5,  16, 3},
		{"Rejuvenation",   SpellElement::Healing,    SpellTarget::Self,  3,  12, 2},
	};
	return catalog;
}

// Look up a spell by exact name from the catalog. Returns nullptr if not found.
inline const Spell* FindSpell(const std::string& name) {
	for (const auto& s : GetSpellCatalog()) {
		if (s.name == name) return &s;
	}
	return nullptr;
}
