// Spell.h
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
	SpellElement element = SpellElement::Arcane;   // default safe value
	SpellTarget target = SpellTarget::Enemy;       // default safe value
	int manaCost = 0;
	int power = 0;             // Damage or heal amount (base)
	int requiredIntelligence = 0; // Minimum INT to learn/use

	// Explicit constructor to guarantee initialization in all code paths
	Spell() = default;
	Spell(const std::string& n, SpellElement e, SpellTarget t, int m, int p, int req)
		: name(n), element(e), target(t), manaCost(m), power(p), requiredIntelligence(req) {
	}

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
		Spell{"Fireball",       SpellElement::Fire,      SpellTarget::Enemy, 3,   6, 2},
		Spell{"Inferno",        SpellElement::Fire,      SpellTarget::Enemy, 6,  12, 4},
		Spell{"Flame Lance",    SpellElement::Fire,      SpellTarget::Enemy, 4,   8, 3},

		// -- Offensive: Ice --
		Spell{"Frost Bolt",     SpellElement::Ice,       SpellTarget::Enemy, 2,   4, 1},
		Spell{"Blizzard",       SpellElement::Ice,       SpellTarget::Enemy, 7,  14, 5},
		Spell{"Ice Shard",      SpellElement::Ice,       SpellTarget::Enemy, 3,   6, 2},

		// -- Offensive: Lightning --
		Spell{"Spark",          SpellElement::Lightning,  SpellTarget::Enemy, 1,   3, 1},
		Spell{"Thunderbolt",    SpellElement::Lightning,  SpellTarget::Enemy, 5,  10, 3},
		Spell{"Chain Lightning",SpellElement::Lightning,  SpellTarget::Enemy, 8,  16, 5},

		// -- Offensive: Shadow --
		Spell{"Shadow Bolt",    SpellElement::Shadow,     SpellTarget::Enemy, 3,   5, 2},
		Spell{"Void Blast",     SpellElement::Shadow,     SpellTarget::Enemy, 6,  12, 4},
		Spell{"Soul Drain",     SpellElement::Shadow,     SpellTarget::Enemy, 4,   7, 3},

		// -- Offensive: Arcane --
		Spell{"Magic Missile",  SpellElement::Arcane,     SpellTarget::Enemy, 1,   3, 1},
		Spell{"Arcane Burst",   SpellElement::Arcane,     SpellTarget::Enemy, 5,  10, 3},

		// -- Healing --
		Spell{"Heal",           SpellElement::Healing,    SpellTarget::Self,  2,   8, 1},
		Spell{"Greater Heal",   SpellElement::Healing,    SpellTarget::Self,  5,  16, 3},
		Spell{"Rejuvenation",   SpellElement::Healing,    SpellTarget::Self,  3,  12, 2},
	};
	return catalog;
}

inline const Spell* FindSpell(const std::string& name) {
	for (const auto& s : GetSpellCatalog()) {
		if (s.name == name) return &s;
	}
	return nullptr;
}