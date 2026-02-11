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

// ?? Master Spell Catalog ??
inline const std::vector<Spell>& GetSpellCatalog() {
	static const std::vector<Spell> catalog = {
		// Offensive — Fire
		{"Fireball",       SpellElement::Fire,      SpellTarget::Enemy, 4,  15, 2},
		{"Inferno",        SpellElement::Fire,      SpellTarget::Enemy, 8,  30, 4},
		{"Flame Lance",    SpellElement::Fire,      SpellTarget::Enemy, 6,  22, 3},

		// Offensive — Ice
		{"Frost Bolt",     SpellElement::Ice,       SpellTarget::Enemy, 3,  12, 1},
		{"Blizzard",       SpellElement::Ice,       SpellTarget::Enemy, 9,  35, 5},
		{"Ice Shard",      SpellElement::Ice,       SpellTarget::Enemy, 5,  18, 2},

		// Offensive — Lightning
		{"Spark",          SpellElement::Lightning,  SpellTarget::Enemy, 2,   8, 1},
		{"Thunderbolt",    SpellElement::Lightning,  SpellTarget::Enemy, 7,  28, 3},
		{"Chain Lightning",SpellElement::Lightning,  SpellTarget::Enemy, 10, 40, 5},

		// Offensive — Shadow
		{"Shadow Bolt",    SpellElement::Shadow,     SpellTarget::Enemy, 4,  14, 2},
		{"Void Blast",     SpellElement::Shadow,     SpellTarget::Enemy, 8,  32, 4},
		{"Soul Drain",     SpellElement::Shadow,     SpellTarget::Enemy, 6,  20, 3},

		// Offensive — Arcane
		{"Magic Missile",  SpellElement::Arcane,     SpellTarget::Enemy, 2,  10, 1},
		{"Arcane Burst",   SpellElement::Arcane,     SpellTarget::Enemy, 7,  26, 3},

		// Healing
		{"Heal",           SpellElement::Healing,    SpellTarget::Self,  3,  20, 1},
		{"Greater Heal",   SpellElement::Healing,    SpellTarget::Self,  6,  40, 3},
		{"Rejuvenation",   SpellElement::Healing,    SpellTarget::Self,  5,  30, 2},
	};
	return catalog;
}

// Find a spell by name from the catalog
inline const Spell* FindSpell(const std::string& name) {
	for (const auto& s : GetSpellCatalog()) {
		if (s.name == name) return &s;
	}
	return nullptr;
}
