#pragma once

#include <string>
#include <vector>

enum class SpellElement { Fire, Ice, Lightning, Healing, Shadow, Arcane };
enum class SpellTarget { Enemy, AllEnemies, ChainEnemies, Self };
enum class SpellEffect { Damage, Heal, Regeneration, Empower };
enum class SpellId {
	Custom,
	Fireball, Inferno, FlameLance,
	FrostBolt, Blizzard, IceShard,
	Spark, Thunderbolt, ChainLightning,
	ShadowBolt, VoidBlast, SoulDrain,
	CorruptingBreath,
	MagicMissile, ArcaneBurst,
	Heal, GreaterHeal, Rejuvenation, Empower
};

struct Spell {
	std::string name;
	SpellId id = SpellId::Custom;
	SpellElement element = SpellElement::Arcane;
	SpellTarget target = SpellTarget::Enemy;
	int manaCost = 0;
	int power = 0;
	int requiredIntelligence = 0;
	SpellEffect effect = SpellEffect::Damage;
	int duration = 0;

	Spell() = default;
	Spell(const std::string& n, SpellElement e, SpellTarget t, int m, int p, int req)
		: name(n), element(e), target(t), manaCost(m), power(p), requiredIntelligence(req),
		  effect(t == SpellTarget::Self ? SpellEffect::Heal : SpellEffect::Damage) {}
	Spell(const std::string& n, SpellElement e, SpellTarget t, int m, int p, int req,
		SpellEffect spellEffect, int effectDuration)
		: name(n), element(e), target(t), manaCost(m), power(p), requiredIntelligence(req),
		  effect(spellEffect), duration(effectDuration) {}
	Spell(SpellId spellId, const std::string& n, SpellElement e, SpellTarget t,
		int m, int p, int req, SpellEffect spellEffect = SpellEffect::Damage,
		int effectDuration = 0)
		: name(n), id(spellId), element(e), target(t), manaCost(m), power(p),
		  requiredIntelligence(req), effect(spellEffect), duration(effectDuration) {}

	std::string GetEffectSummary() const {
		switch (effect) {
		case SpellEffect::Damage:
			if (target == SpellTarget::AllEnemies) return "Damage to all enemies";
			if (target == SpellTarget::ChainEnemies) return "Chaining damage";
			return "Focused damage";
		case SpellEffect::Heal: return "Immediate healing";
		case SpellEffect::Regeneration: return "Healing over time";
		case SpellEffect::Empower:
			return "+" + std::to_string(power) + "% damage, next "
				+ std::to_string(duration) + " hits";
		}
		return "Unknown effect";
	}

	std::string GetElementName() const {
		switch (element) {
		case SpellElement::Fire: return "Fire";
		case SpellElement::Ice: return "Ice";
		case SpellElement::Lightning: return "Lightning";
		case SpellElement::Healing: return "Healing";
		case SpellElement::Shadow: return "Shadow";
		case SpellElement::Arcane: return "Arcane";
		}
		return "Unknown";
	}
};

inline const std::vector<Spell>& GetSpellCatalog() {
	static const std::vector<Spell> catalog = {
		{SpellId::Fireball, "Fireball", SpellElement::Fire, SpellTarget::Enemy, 3, 6, 2},
		{SpellId::Inferno, "Inferno", SpellElement::Fire, SpellTarget::AllEnemies, 6, 12, 4},
		{SpellId::FlameLance, "Flame Lance", SpellElement::Fire, SpellTarget::Enemy, 4, 8, 3},
		{SpellId::FrostBolt, "Frost Bolt", SpellElement::Ice, SpellTarget::Enemy, 2, 4, 1},
		{SpellId::Blizzard, "Blizzard", SpellElement::Ice, SpellTarget::AllEnemies, 7, 14, 5},
		{SpellId::IceShard, "Ice Shard", SpellElement::Ice, SpellTarget::Enemy, 3, 6, 2},
		{SpellId::Spark, "Spark", SpellElement::Lightning, SpellTarget::Enemy, 1, 3, 1},
		{SpellId::Thunderbolt, "Thunderbolt", SpellElement::Lightning, SpellTarget::Enemy, 5, 10, 3},
		{SpellId::ChainLightning, "Chain Lightning", SpellElement::Lightning, SpellTarget::ChainEnemies, 8, 16, 5},
		{SpellId::ShadowBolt, "Shadow Bolt", SpellElement::Shadow, SpellTarget::Enemy, 3, 5, 2},
		{SpellId::VoidBlast, "Void Blast", SpellElement::Shadow, SpellTarget::Enemy, 6, 12, 4},
		{SpellId::SoulDrain, "Soul Drain", SpellElement::Shadow, SpellTarget::Enemy, 4, 7, 3},
		{SpellId::CorruptingBreath, "Corrupting Breath", SpellElement::Shadow, SpellTarget::AllEnemies, 7, 11, 4},
		{SpellId::MagicMissile, "Magic Missile", SpellElement::Arcane, SpellTarget::Enemy, 1, 3, 1},
		{SpellId::ArcaneBurst, "Arcane Burst", SpellElement::Arcane, SpellTarget::AllEnemies, 5, 10, 3},
		{SpellId::Heal, "Heal", SpellElement::Healing, SpellTarget::Self, 2, 8, 1, SpellEffect::Heal},
		{SpellId::GreaterHeal, "Greater Heal", SpellElement::Healing, SpellTarget::Self, 5, 16, 3, SpellEffect::Heal},
		{SpellId::Rejuvenation, "Rejuvenation", SpellElement::Healing, SpellTarget::Self, 3, 12, 2, SpellEffect::Regeneration, 3},
		{SpellId::Empower, "Empower", SpellElement::Arcane, SpellTarget::Self, 2, 25, 1, SpellEffect::Empower, 2},
	};
	return catalog;
}

inline const Spell* FindSpell(const std::string& name) {
	for (const Spell& spell : GetSpellCatalog()) if (spell.name == name) return &spell;
	return nullptr;
}
