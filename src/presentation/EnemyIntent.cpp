#include "EnemyIntent.h"
#include "combat/DefenseRules.h"

#include <algorithm>

namespace {

std::string AttackName(AttackStyle style) {
	switch (style) {
	case AttackStyle::Slash: return "SLASH";
	case AttackStyle::Thrust: return "THRUST";
	case AttackStyle::Bash: return "BASH";
	}
	return "ATTACK";
}

std::string DefenseName(DefenseStance stance) {
	switch (stance) {
	case DefenseStance::AntiSlash: return "SLASH";
	case DefenseStance::AntiThrust: return "THRUST";
	case DefenseStance::AntiBash: return "BASH";
	case DefenseStance::AntiMagic: return "MAGIC";
	}
	return "ATTACKS";
}

std::string AttackHint(const std::string& name, AttackStyle style) {
	switch (style) {
	case AttackStyle::Slash:
		return name + " draws back for a broad, sweeping strike.";
	case AttackStyle::Thrust:
		return name + " narrows its stance and lines up your center.";
	case AttackStyle::Bash:
		return name + " plants its feet and raises its weight high.";
	}
	return name + " prepares to attack.";
}

std::string DefenseHint(const std::string& name, DefenseStance stance) {
	switch (stance) {
	case DefenseStance::AntiSlash:
		return name + " spreads its guard wide against sweeping blows.";
	case DefenseStance::AntiThrust:
		return name + " closes its guard tightly around its center line.";
	case DefenseStance::AntiBash:
		return name + " lowers its center of gravity and braces hard.";
	case DefenseStance::AntiMagic:
		return "A faint ward gathers around " + name + ".";
	}
	return name + " raises its guard.";
}

std::string SpellHint(const std::string& name, SpellElement element) {
	switch (element) {
	case SpellElement::Fire: return "Heat gathers around " + name + ".";
	case SpellElement::Ice: return "Frost creeps through the air around " + name + ".";
	case SpellElement::Lightning: return "The air crackles around " + name + ".";
	case SpellElement::Healing: return name + " draws restorative energy inward.";
	case SpellElement::Shadow: return "The shadows bend toward " + name + ".";
	case SpellElement::Arcane: return "Arcane light gathers in " + name + "'s hands.";
	}
	return name + " gathers magical energy.";
}

} // namespace

namespace EnemyIntent {

IntentClarity DetermineClarity(EnemyKnowledge knowledge, int playerIntelligence) {
	int rank = static_cast<int>(knowledge);
	if (playerIntelligence >= 4) ++rank;
	if (playerIntelligence >= 8) ++rank;
	rank = std::clamp(rank, 0, 3);
	return static_cast<IntentClarity>(rank);
}

std::string Describe(const Enemy& enemy, const TurnAction& action,
	IntentClarity clarity) {
	const std::string& name = enemy.GetName();

	switch (action.type) {
	case ActionType::Attack:
		if (clarity == IntentClarity::Veiled) {
			return name + " shifts forward with violent intent.";
		}
		if (clarity == IntentClarity::Hinted) {
			return AttackHint(name, action.attackStyle) + " Guard: "
				+ DefenseRules::DescribeChallenge(action, nullptr,
					enemy.GetSpeed(), enemy.GetATK()) + ".";
		}
		return AttackName(action.attackStyle) + " - " + AttackHint(name, action.attackStyle)
			+ " Guard: " + DefenseRules::DescribeChallenge(
				action, nullptr, enemy.GetSpeed(), enemy.GetATK()) + ".";

	case ActionType::Defend:
		if (clarity == IntentClarity::Veiled) {
			return name + " watches your movements and raises its guard.";
		}
		if (clarity == IntentClarity::Hinted) {
			return DefenseHint(name, action.defenseStance);
		}
		return "GUARD: " + DefenseName(action.defenseStance) + " - "
			+ DefenseHint(name, action.defenseStance);

	case ActionType::CastSpell: {
		const auto& spells = enemy.GetKnownSpells();
		if (action.spellIndex < 0 || action.spellIndex >= static_cast<int>(spells.size())) {
			return name + " gathers unstable magical energy.";
		}
		const Spell& spell = spells[action.spellIndex];
		if (clarity == IntentClarity::Veiled) {
			return name + " begins shaping a spell.";
		}
		if (clarity == IntentClarity::Hinted) {
			return SpellHint(name, spell.element)
				+ (spell.effect == SpellEffect::Damage
					? " Guard: " + DefenseRules::DescribeChallenge(
						action, &spell, enemy.GetSpeed(), enemy.GetATK()) + "."
					: "");
		}
		if (clarity == IntentClarity::Clear) {
			return spell.GetElementName() + " MAGIC - " + SpellHint(name, spell.element)
				+ (spell.effect == SpellEffect::Damage
					? " Guard: " + DefenseRules::DescribeChallenge(
						action, &spell, enemy.GetSpeed(), enemy.GetATK()) + "."
					: "");
		}
		return spell.name + " - " + SpellHint(name, spell.element)
			+ (spell.effect == SpellEffect::Damage
				? " Guard: " + DefenseRules::DescribeChallenge(
					action, &spell, enemy.GetSpeed(), enemy.GetATK()) + "."
				: "");
	}

	case ActionType::UseItem:
		return name + " reaches for an item.";
	case ActionType::Inspect:
		return name + " studies you carefully.";
	case ActionType::None:
		return name + " hesitates.";
	}

	return name + " is difficult to read.";
}

} // namespace EnemyIntent
