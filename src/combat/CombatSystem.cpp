#include "CombatSystem.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include <iostream>
#include <algorithm>

static void ExecuteAction(Entity& actor, Entity& target, const TurnAction& action) {
	actor.SetDefending(false); // Reset defending each action

	switch (action.type) {
	case ActionType::Attack: {
		int dmg = actor.GetATK();
		std::cout << "  " << actor.GetName() << " attacks for " << dmg << " damage!";
		target.ReceiveDamage(dmg);
		if (target.IsDefending()) std::cout << " (halved by defense!)";
		std::cout << " [" << target.GetName() << ": " << target.GetHP() << "/" << target.GetMaxHP() << " HP]\n";
		break;
	}
	case ActionType::Defend:
		actor.SetDefending(true);
		std::cout << "  " << actor.GetName() << " is defending! (Damage halved next hit)\n";
		break;
	case ActionType::CastSpell: {
		const auto& spells = actor.GetKnownSpells();
		if (action.spellIndex < 0 || action.spellIndex >= static_cast<int>(spells.size())) {
			std::cout << "  Spell fizzles...\n";
			break;
		}
		const Spell& spell = spells[action.spellIndex];
		actor.UseMana(spell.manaCost);

		if (spell.target == SpellTarget::Enemy) {
			// Damage spell: power scaled by intelligence
			int dmg = spell.power + actor.GetIntelligence() * 2;
			std::cout << "  " << actor.GetName() << " casts " << spell.name
				<< " [" << spell.GetElementName() << "] for " << dmg << " damage!";
			target.ReceiveDamage(dmg);
			if (target.IsDefending()) std::cout << " (halved by defense!)";
			std::cout << " [" << target.GetName() << ": " << target.GetHP() << "/" << target.GetMaxHP() << " HP]\n";
		}
		else {
			// Self-targeting (heal)
			int heal = spell.power + actor.GetIntelligence();
			actor.Heal(heal);
			std::cout << "  " << actor.GetName() << " casts " << spell.name
				<< " and restores " << heal << " HP!"
				<< " [" << actor.GetHP() << "/" << actor.GetMaxHP() << " HP]\n";
		}
		std::cout << "  [" << actor.GetName() << " Mana: " << actor.GetMana() << "/" << actor.GetMaxMana() << "]\n";
		break;
	}
	case ActionType::UseItem:
		// Handled by the Player specifically before this function
		break;
	case ActionType::None:
		std::cout << "  " << actor.GetName() << " does nothing.\n";
		break;
	}
}

bool CombatSystem::ResolveCombat(Player& player, Enemy& enemy) {
	std::cout << "\n??????????????????????????????????\n";
	std::cout << "  A " << enemy.GetName() << " appears!\n";
	std::cout << "??????????????????????????????????\n";
	enemy.PrintStatus();
	std::cout << "\n";

	while (player.IsAlive() && enemy.IsAlive()) {
		// ?? Speed-based turn calculation ??
		int playerSpeed = player.GetSpeed();
		int enemySpeed = enemy.GetSpeed();

		int playerActions = player.ActionsPerRound(enemySpeed);
		int enemyActions = enemy.ActionsPerRound(playerSpeed);

		std::cout << "\n?? Round Start ??\n";
		player.PrintStatus();
		enemy.PrintStatus();

		// Determine who goes first (higher speed)
		bool playerFirst = playerSpeed >= enemySpeed;

		auto doPlayerActions = [&]() {
			for (int i = 0; i < playerActions && player.IsAlive() && enemy.IsAlive(); ++i) {
				if (playerActions > 1)
					std::cout << "  [Action " << (i + 1) << "/" << playerActions << "]\n";

				TurnAction action = player.DecideTurn();

				// Handle item use specially (needs inventory access)
				if (action.type == ActionType::UseItem) {
					int hp = player.GetHP();
					int mana = player.GetMana();
					player.GetInventory().UseItem(
						action.itemIndex, hp, player.GetMaxHP(), mana, player.GetMaxMana());
					// Apply the changes via Entity methods
					if (hp > player.GetHP()) player.Heal(hp - player.GetHP());
					if (mana > player.GetMana()) player.RestoreMana(mana - player.GetMana());
				}
				else {
					ExecuteAction(player, enemy, action);
				}
			}
		};

		auto doEnemyActions = [&]() {
			for (int i = 0; i < enemyActions && player.IsAlive() && enemy.IsAlive(); ++i) {
				if (enemyActions > 1)
					std::cout << "  [Enemy Action " << (i + 1) << "/" << enemyActions << "]\n";

				TurnAction action = enemy.DecideTurn();
				ExecuteAction(enemy, player, action);
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

		// Reset defending at end of round
		player.SetDefending(false);
		enemy.SetDefending(false);
	}

	if (player.IsAlive()) {
		std::cout << "\n  ** " << enemy.GetName() << " has been defeated! **\n";

		// Try to learn a spell from the enemy
		for (const auto& spell : enemy.GetKnownSpells()) {
			player.TryLearnSpell(spell);
		}
		return true;
	}
	else {
		std::cout << "\n  " << player.GetName() << " has fallen in combat...\n";
		return false;
	}
}
