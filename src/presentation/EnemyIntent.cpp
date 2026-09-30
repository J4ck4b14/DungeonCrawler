#include "EnemyIntent.h"
#include <algorithm>
#include <array>

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

std::string AttackCategory(AttackStyle style) {
	switch (style) {
	case AttackStyle::Slash: return "a broad physical commitment";
	case AttackStyle::Thrust: return "a focused forward attack";
	case AttackStyle::Bash: return "a heavy physical commitment";
	}
	return "a physical attack";
}

std::string AttackPrediction(AttackStyle style) {
	switch (style) {
	case AttackStyle::Slash: return "That stance usually precedes a sweeping strike.";
	case AttackStyle::Thrust: return "That alignment usually precedes a driving thrust.";
	case AttackStyle::Bash: return "That raised weight usually precedes a crushing blow.";
	}
	return "That stance usually precedes an attack.";
}

std::string HesitationHint(const Enemy& enemy) {
	static constexpr std::array<const char*,
		static_cast<std::size_t>(EnemyArchetype::Count)> tells = {{
		"compresses inward, swelling before its next commitment.",
		"darts out of reach, already searching for another opening.",
		"holds perfectly still for one metronomic beat.",
		"skitters laterally, testing the shape of your guard.",
		"fakes a retreat and watches whether you chase.",
		"resets its footing without dropping its measured guard.",
		"must recover after throwing its full weight forward.",
		"fades almost completely from view for a strange heartbeat.",
		"lets one pulse of its casting rhythm pass in silence.",
		"drags in a slow breath while torn flesh begins to knit.",
		"circles in a frantic pause before the next burst.",
		"waits with unnerving patience for you to overcommit.",
		"halts at an exact, artificial interval in its spell pattern.",
		"smolders, gathering momentum for renewed violence.",
		"draws back with painfully obvious, ponderous intent.",
		"surveys the field before combining its next threats."
	}};
	return enemy.GetName() + " "
		+ tells[static_cast<std::size_t>(enemy.GetArchetype())];
}

std::string VeiledTell(const Enemy& enemy) {
	static constexpr std::array<const char*,
		static_cast<std::size_t>(EnemyArchetype::Count)> tells = {{
		"compresses and shifts without revealing when it will spring.",
		"twitches at the edge of striking distance.",
		"settles into a rigid posture.",
		"weaves sideways across your sightline.",
		"shows you a movement that may be a lie.",
		"adjusts its measured guard without giving away the finish.",
		"shifts its weight into a forceful commitment.",
		"flickers at the edge of visibility.",
		"marks a quiet beat in a wicked cadence.",
		"rolls its shoulders into a broad, slow motion.",
		"circles in a volatile hunting crouch.",
		"waits with poised, unreadable patience.",
		"moves at a precise but alien interval.",
		"surges forward as the air grows hotter.",
		"draws back with enormous, unmistakable weight.",
		"gathers several threats into one deliberate motion."
	}};
	return enemy.GetName() + " "
		+ tells[static_cast<std::size_t>(enemy.GetArchetype())];
}

int SpeciesReadability(const Enemy& enemy, const TurnAction& action,
	bool committed) {
	switch (enemy.GetArchetype()) {
	case EnemyArchetype::Giant: return 2;
	case EnemyArchetype::Skeleton: return 1;
	case EnemyArchetype::Orc: return committed ? 1 : 0;
	case EnemyArchetype::Bandit: return -2;
	case EnemyArchetype::Goblin:
	case EnemyArchetype::Werewolf:
	case EnemyArchetype::Ghost: return -1;
	case EnemyArchetype::Witch:
		return action.type == ActionType::CastSpell ? 1 : 0;
	case EnemyArchetype::DarkMage:
		return action.type == ActionType::Attack ? -2 : -1;
	case EnemyArchetype::Dragon:
	case EnemyArchetype::Count: return 0;
	default: return 0;
	}
}

} // namespace

namespace EnemyIntent {

IntentClarity DetermineClarity(const Enemy& enemy, const TurnAction& action,
	EnemyKnowledge knowledge, int playerIntelligence, bool committed) {
	// Clarity combines persistent knowledge, the current hero's INT, how readable
	// this species is, and whether the enemy has actually committed. The final
	// gate matters: good knowledge predicts well, but it does not become prophecy.
	int score = static_cast<int>(knowledge);
	if (playerIntelligence >= 4) ++score;
	if (playerIntelligence >= 9) ++score;
	if (playerIntelligence >= 16) ++score;
	score += SpeciesReadability(enemy, action, committed);
	if (committed) ++score;

	if (score <= 0) return IntentClarity::Veiled;
	if (score == 1) return IntentClarity::Alert;
	if (score == 2) return IntentClarity::Hinted;
	if (score < 5 || !committed) return IntentClarity::Clear;
	return IntentClarity::Exact;
}

std::string Describe(const Enemy& enemy, const TurnAction& action,
	IntentClarity clarity) {
	const std::string& name = enemy.GetName();

	switch (action.type) {
	case ActionType::Attack:
		if (clarity == IntentClarity::Veiled) {
			return VeiledTell(enemy);
		}
		if (clarity == IntentClarity::Alert)
			return name + " seems ready to attack.";
		if (clarity == IntentClarity::Hinted) {
			return name + " looks prepared for "
				+ AttackCategory(action.attackStyle) + ".";
		}
		if (clarity == IntentClarity::Clear) return AttackPrediction(action.attackStyle);
		return "COMMITTED: " + AttackName(action.attackStyle) + " - "
			+ AttackHint(name, action.attackStyle);

	case ActionType::Defend:
		if (clarity == IntentClarity::Veiled) {
			return VeiledTell(enemy);
		}
		if (clarity == IntentClarity::Alert)
			return name + " seems ready to guard.";
		if (clarity == IntentClarity::Hinted) {
			return name + " appears to be preparing a specialized guard.";
		}
		if (clarity == IntentClarity::Clear)
			return DefenseHint(name, action.defenseStance);
		return "COMMITTED GUARD: " + DefenseName(action.defenseStance) + " - "
			+ DefenseHint(name, action.defenseStance);

	case ActionType::CastSpell: {
		const auto& spells = enemy.GetKnownSpells();
		if (action.spellIndex < 0 || action.spellIndex >= static_cast<int>(spells.size())) {
			return name + " gathers unstable magical energy.";
		}
		const Spell& spell = spells[action.spellIndex];
		if (clarity == IntentClarity::Veiled) {
			return VeiledTell(enemy);
		}
		if (clarity == IntentClarity::Alert)
			return name + " seems ready to cast.";
		if (clarity == IntentClarity::Hinted) {
			return spell.effect == SpellEffect::Damage
				? name + " looks ready to release " + spell.GetElementName() + " magic."
				: name + " appears to be shaping supportive magic.";
		}
		if (clarity == IntentClarity::Clear) {
			return "That cadence usually precedes " + spell.GetElementName()
				+ " magic. " + SpellHint(name, spell.element);
		}
		return "COMMITTED: " + spell.name + " - "
			+ SpellHint(name, spell.element);
	}

	case ActionType::UseItem:
		return name + " reaches for an item.";
	case ActionType::Inspect:
		return name + " studies you carefully.";
	case ActionType::None:
		return HesitationHint(enemy);
	}

	return name + " is difficult to read.";
}

} // namespace EnemyIntent
