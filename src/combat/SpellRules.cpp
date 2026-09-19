#include "SpellRules.h"

#include <algorithm>
#include <cmath>

namespace {
int FloorPositive(double value) {
	return std::max(1, static_cast<int>(std::floor(value)));
}

double RankChance(int rank, double base, double perRank, double cap) {
	return std::min(cap, base + std::max(0, rank - 1) * perRank);
}
} // namespace

namespace SpellRules {

int CalculateDamage(const Spell& spell, int strength, int speed,
	int intelligence, int casterRank, int jumpIndex) {
	(void)casterRank;
	double damage = spell.power + intelligence * 2.0;
	switch (spell.id) {
	case SpellId::Fireball: damage = 6.0 + intelligence * 1.8; break;
	case SpellId::Inferno: damage = 8.0 + intelligence * 1.35; break;
	case SpellId::FlameLance:
		damage = 7.0 + intelligence * 1.65 + strength * .45; break;
	case SpellId::FrostBolt: damage = 4.0 + intelligence * 1.45; break;
	case SpellId::Blizzard: damage = 5.0 + intelligence * 1.05; break;
	case SpellId::IceShard: damage = 6.0 + intelligence * 1.75; break;
	case SpellId::Spark: damage = 3.0 + intelligence * 1.15 + speed * .40; break;
	case SpellId::Thunderbolt: damage = 9.0 + intelligence * 2.20; break;
	case SpellId::ChainLightning: damage = 8.0 + intelligence * 1.70; break;
	case SpellId::ShadowBolt: damage = 5.0 + intelligence * 1.65; break;
	case SpellId::VoidBlast: damage = 14.0 + intelligence * 2.35; break;
	case SpellId::SoulDrain: damage = 6.0 + intelligence * 1.45; break;
	case SpellId::CorruptingBreath: damage = 7.0 + intelligence * 1.20; break;
	case SpellId::MagicMissile: damage = 3.0 + intelligence * 1.20; break;
	case SpellId::ArcaneBurst: damage = 6.0 + intelligence * 1.15; break;
	default: break;
	}
	if (spell.id == SpellId::ChainLightning && jumpIndex > 0) {
		damage *= std::pow(.72, jumpIndex);
	}
	return FloorPositive(damage);
}

int CalculateHealing(const Spell& spell, int maximumHp, int intelligence) {
	(void)maximumHp;
	if (spell.id == SpellId::GreaterHeal) return FloorPositive(16.0 + intelligence * 2.0);
	return FloorPositive(8.0 + intelligence * 1.4);
}

int RegenerationStacks(const Spell& spell, int maximumHp, int intelligence) {
	if (spell.id != SpellId::Rejuvenation) return 0;
	return std::max(1, (intelligence + maximumHp / 10) / 4);
}

int DrainHealing(const Spell& spell, int damageDealt) {
	if (spell.id != SpellId::SoulDrain || damageDealt <= 0) return 0;
	return std::max(1, damageDealt * 2 / 5);
}

int DamageAgainstConventionalGuard(const Spell& spell, int damage) {
	if (damage <= 0) return 0;
	double bypass = 0.0;
	if (spell.id == SpellId::IceShard) bypass = .65;
	else if (spell.id == SpellId::ShadowBolt) bypass = .40;
	else if (spell.id == SpellId::MagicMissile) bypass = .50;
	const int normallyBlocked = damage - damage / 2;
	return damage / 2 + static_cast<int>(std::floor(normallyBlocked * bypass));
}

std::optional<SpellStatusRule> StatusRule(const Spell& spell, int casterRank) {
	casterRank = std::clamp(casterRank, 1, 50);
	switch (spell.id) {
	case SpellId::Fireball:
		return SpellStatusRule{StatusType::Burn,
			RankChance(casterRank, .30, .005, .65), 1 + (casterRank - 1) / 15, 3};
	case SpellId::Inferno:
		return SpellStatusRule{StatusType::Burn,
			RankChance(casterRank, .65, .004, .90), 2 + (casterRank - 1) / 12, 3};
	case SpellId::FrostBolt:
		return SpellStatusRule{StatusType::Freeze,
			RankChance(casterRank, .25, .004, .55), 1, 1};
	case SpellId::Blizzard:
		return SpellStatusRule{StatusType::Freeze,
			RankChance(casterRank, .18, .003, .42), 1, 1};
	case SpellId::Thunderbolt:
		return SpellStatusRule{StatusType::Freeze,
			RankChance(casterRank, .20, .003, .40), 1, 1};
	case SpellId::ChainLightning:
		return SpellStatusRule{StatusType::Freeze,
			RankChance(casterRank, .10, .002, .28), 1, 1};
	case SpellId::CorruptingBreath:
		return SpellStatusRule{StatusType::Poison,
			RankChance(casterRank, .28, .004, .48),
			1 + (casterRank - 1) / 16, 3};
	default: return std::nullopt;
	}
}

bool IsMultiTarget(const Spell& spell) { return spell.target == SpellTarget::AllEnemies; }
bool IsChain(const Spell& spell) { return spell.target == SpellTarget::ChainEnemies; }
int AffectedTargetCount(const Spell& spell, int livingEnemyCount) {
	livingEnemyCount = std::max(0, livingEnemyCount);
	if (IsMultiTarget(spell) || IsChain(spell)) return livingEnemyCount;
	return std::min(1, livingEnemyCount);
}
bool SuppressesRegeneration(const Spell& spell) {
	return spell.element == SpellElement::Fire && spell.effect == SpellEffect::Damage;
}

} // namespace SpellRules
