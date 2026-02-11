#include "CombatSystem.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "core/GameStats.h"
#include "utils/RNG.h"
#include "utils/Console.h"
#include <iostream>
#include <algorithm>
#include <set>

static void ExecuteAction(Entity& actor, Entity& target, const TurnAction& action,
	GameStats& stats, Enemy* enemyTarget = nullptr, bool isPlayer = false) {
	actor.SetDefending(false); // Reset defending each action

	switch (action.type) {
	case ActionType::Attack: {
		int baseAtk = actor.GetATK();
		int dmg = baseAtk;
		std::string styleDesc;

		switch (action.attackStyle) {
		case AttackStyle::Slash:
			dmg = baseAtk;
			styleDesc = "slashes";
			break;
		case AttackStyle::Thrust:
			dmg = static_cast<int>(baseAtk * 0.8f);
			if (dmg < 1) dmg = 1;
			styleDesc = "thrusts precisely";
			break;
		case AttackStyle::Bash:
			dmg = static_cast<int>(baseAtk * 1.3f);
			styleDesc = "bashes heavily";
			break;
		}

		// Thrust ignores defense
		bool ignoreDefense = (action.attackStyle == AttackStyle::Thrust);

		if (isPlayer) stats.RecordPhysicalAttack();
		if (isPlayer) stats.totalDamageDealt += dmg;
		if (!isPlayer) stats.totalDamageTaken += dmg;

		Console::PrintSlow("  " + actor.GetName() + " " + styleDesc + " for "
			+ std::to_string(dmg) + " damage!");

		if (ignoreDefense) {
			// Temporarily disable defense for this hit
			bool wasDefending = target.IsDefending();
			target.SetDefending(false);
			target.ReceiveDamage(dmg);
			target.SetDefending(wasDefending);
			if (wasDefending)
				Console::PrintSlow("  (Thrust pierces through the defense!)");
		}
		else {
			target.ReceiveDamage(dmg);
			if (target.IsDefending())
				Console::PrintSlow("  (Halved by defense!)");
		}
		break;
	}
	case ActionType::Defend:
		actor.SetDefending(true);
		Console::PrintSlow("  " + actor.GetName() + " is defending! (Damage halved next hit)");
		break;
	case ActionType::CastSpell: {
		const auto& spells = actor.GetKnownSpells();
		if (action.spellIndex < 0 || action.spellIndex >= static_cast<int>(spells.size())) {
			Console::PrintSlow("  Spell fizzles...");
			break;
		}
		const Spell& spell = spells[action.spellIndex];
		actor.UseMana(spell.manaCost);

		if (isPlayer) stats.RecordSpellCast(spell.name);

		if (spell.target == SpellTarget::Enemy) {
			int dmg = spell.power + actor.GetIntelligence() * 2;

			// Check for weakness bonus
			if (enemyTarget && spell.element == enemyTarget->GetWeakness()) {
				dmg = static_cast<int>(dmg * 1.5f);
				Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
					+ " [" + spell.GetElementName() + "] for " + std::to_string(dmg)
					+ " damage! It's super effective!");
			}
			else {
				std::string msg = "  " + actor.GetName() + " casts " + spell.name
					+ " [" + spell.GetElementName() + "] for " + std::to_string(dmg) + " damage!";
				if (target.IsDefending()) msg += " (halved by defense!)";
				Console::PrintSlow(msg);
			}
			target.ReceiveDamage(dmg);

			if (isPlayer) stats.totalDamageDealt += dmg;
			if (!isPlayer) stats.totalDamageTaken += dmg;
		}
		else {
			int heal = spell.power + actor.GetIntelligence();
			actor.Heal(heal);
			if (isPlayer) stats.totalHealing += heal;
			Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
				+ " and restores " + std::to_string(heal) + " HP!");
		}
		break;
	}
	case ActionType::UseItem:
		break;
	case ActionType::Inspect:
		break;
	case ActionType::None:
		Console::PrintSlow("  " + actor.GetName() + " does nothing.");
		break;
	}
}

static EnemyKnowledge InspectEnemy(const Player& player, Enemy& enemy,
	EnemyKnowledge currentKnowledge) {
	static RNG rng;
	int base = rng.NextInt(1, 20);
	int total = base + player.GetIntelligence();

	if (base == 1) {
		Console::PrintSlow("  You try to study the " + enemy.GetName()
			+ "... but you can't focus at all. It's a blur.");
		return currentKnowledge;
	}
	else if (total < 10) {
		Console::PrintSlow("  You squint at the " + enemy.GetName()
			+ "... you can't make out much.");
		if (currentKnowledge < EnemyKnowledge::Approximate)
			return EnemyKnowledge::Approximate;
		return currentKnowledge;
	}
	else if (total < 18) {
		Console::PrintSlow("  You study the " + enemy.GetName()
			+ " carefully and get a read on it.");
		if (currentKnowledge < EnemyKnowledge::Partial)
			return EnemyKnowledge::Partial;
		return currentKnowledge;
	}
	else {
		Console::PrintSlow("  You lock eyes with the " + enemy.GetName()
			+ " and see through it completely.");
		return EnemyKnowledge::Full;
	}
}

bool CombatSystem::ResolveCombat(Player& player, Enemy& enemy,
	std::set<std::string>& seenEnemyTypes, GameStats& gameStats) {

	Console::Clear();

	EnemyKnowledge knowledge = EnemyKnowledge::None;
	if (seenEnemyTypes.count(enemy.GetName())) {
		knowledge = EnemyKnowledge::Approximate;
	}

	Console::PrintSlow("\n==================================");
	Console::PrintSlow("  A " + enemy.GetName() + " appears!");
	Console::PrintSlow("==================================");
	enemy.PrintStatus(knowledge);
	std::cout << "\n";

	while (player.IsAlive() && enemy.IsAlive()) {
		int playerSpeed = player.GetSpeed();
		int enemySpeed = enemy.GetSpeed();

		int playerActions = player.ActionsPerRound(enemySpeed);
		int enemyActions = enemy.ActionsPerRound(playerSpeed);

		std::cout << "\n-- Round Start --\n";
		player.PrintStatus();
		enemy.PrintStatus(knowledge);

		bool playerFirst = playerSpeed >= enemySpeed;

		auto doPlayerActions = [&]() {
			for (int i = 0; i < playerActions && player.IsAlive() && enemy.IsAlive(); ++i) {
				if (playerActions > 1)
					std::cout << "  [Action " << (i + 1) << "/" << playerActions << "]\n";

				TurnAction action = player.DecideTurn();

				if (action.type == ActionType::Inspect) {
					knowledge = InspectEnemy(player, enemy, knowledge);
					enemy.PrintStatus(knowledge);
				}
				else if (action.type == ActionType::UseItem) {
					int hp = player.GetHP();
					int mana = player.GetMana();
					player.GetInventory().UseItem(
						action.itemIndex, hp, player.GetMaxHP(), mana, player.GetMaxMana());
					if (hp > player.GetHP()) player.Heal(hp - player.GetHP());
					if (mana > player.GetMana()) player.RestoreMana(mana - player.GetMana());
				}
				else {
					ExecuteAction(player, enemy, action, gameStats, &enemy, true);
				}
			}
		};

		auto doEnemyActions = [&]() {
			for (int i = 0; i < enemyActions && player.IsAlive() && enemy.IsAlive(); ++i) {
				if (enemyActions > 1)
					std::cout << "  [Enemy Action " << (i + 1) << "/" << enemyActions << "]\n";

				TurnAction action = enemy.DecideTurn();
				ExecuteAction(enemy, player, action, gameStats, nullptr, false);
			}
		};

		if (playerFirst) {
			doPlayerActions();
			if (enemy.IsAlive()) doEnemyActions();
		}
		else {
			doEnemyActions();
			if (player.IsAlive()) doPlayerActions();
		}

		player.SetDefending(false);
		enemy.SetDefending(false);
	}

	seenEnemyTypes.insert(enemy.GetName());

	if (player.IsAlive()) {
		Console::PrintSlow("\n  ** " + enemy.GetName() + " has been defeated! **");
		gameStats.totalKills++;

		player.GainXP(enemy.GetXPReward());

		for (const auto& spell : enemy.GetKnownSpells()) {
			player.TryLearnSpell(spell);
		}
		return true;
	}
	else {
		Console::PrintSlow("\n  " + player.GetName() + " has fallen in combat...");
		return false;
	}
}
