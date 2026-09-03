#include "CombatMenu.h"

#include "entities/Enemy.h"
#include "entities/Player.h"
#include <iostream>
#include <limits>

namespace {

int ReadChoice(int minimum, int maximum, const char* retryPrompt) {
	int choice = 0;
	std::cin >> choice;
	while (std::cin.fail() || choice < minimum || choice > maximum) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << retryPrompt;
		std::cin >> choice;
	}
	return choice;
}

} // namespace

namespace CombatMenu {

TurnAction ChooseAction(Player& player) {
	while (true) {
		TurnAction action;

		const AttackBuff& buff = player.GetAttackBuff();
		if (buff.remainingHits > 0) {
			std::cout << "  [BUFF: +" << buff.bonusDamage
				<< (buff.isMagical ? " spell" : " physical")
				<< " dmg, " << buff.remainingHits << " hits left]\n";
		}
		const PowerBuff& powerBuff = player.GetPowerBuff();
		if (powerBuff.remainingHits > 0) {
			std::cout << "  [EMPOWERED: +" << powerBuff.percentBonus
				<< "% damage, " << powerBuff.remainingHits << " hits left]\n";
		}

		std::cout << "\n" << player.GetName() << "'s turn:\n"
			<< "  1. Attack\n"
			<< "  2. Defend (react to incoming attacks)\n"
			<< "  3. Cast Spell\n"
			<< "  4. Use Item\n"
			<< "  5. Inspect Enemy\n"
			<< "  6. Bestiary\n"
			<< "  > ";
		int choice = ReadChoice(1, 6, "  Invalid. Enter 1-6: ");

		switch (choice) {
	case 1: {
		std::cout << "  Choose attack style:\n"
			<< "    1. Slash  (1.0x ATK, 15% crit for 1.5x)\n"
			<< "    2. Thrust (0.8x ATK; 1.0x + pierce vs WRONG stance, parryable by Anti-Thrust)\n"
			<< "    3. Bash   (1.3x ATK, 15% whiff + self-damage)\n"
			<< "    0. Back\n"
			<< "    > ";
		int attackChoice = ReadChoice(0, 3, "    Invalid. Enter 0-3: ");
		if (attackChoice == 0) continue;
		action.type = ActionType::Attack;
		switch (attackChoice) {
		case 1:
			action.attackStyle = AttackStyle::Slash;
			std::cout << "  " << player.GetName() << " readies a slashing attack!\n";
			break;
		case 2:
			action.attackStyle = AttackStyle::Thrust;
			std::cout << "  " << player.GetName() << " aims a precise thrust!\n";
			break;
		case 3:
			action.attackStyle = AttackStyle::Bash;
			std::cout << "  " << player.GetName() << " winds up a heavy bash!\n";
			break;
		}
		break;
	}
	case 2: {
		action.type = ActionType::Defend;
		break;
	}
	case 3: {
		const auto& spells = player.GetKnownSpells();
		if (spells.empty()) {
			std::cout << "  You don't know any spells. Choose another action.\n";
			continue;
		}
		std::cout << "  Known spells:\n";
		for (size_t i = 0; i < spells.size(); ++i) {
			const Spell& spell = spells[i];
			std::cout << "    " << (i + 1) << ". " << spell.name
				<< " [" << spell.GetElementName() << "] "
				<< "(Cost: " << player.GetEffectiveManaCost(spell)
				<< " Mana, " << spell.GetEffectSummary() << ")\n";
		}
		std::cout << "    0. Cancel\n  > ";
		int spellChoice = ReadChoice(0, static_cast<int>(spells.size()),
			"  Invalid spell. Try again: ");
		if (spellChoice == 0) continue;
		int index = spellChoice - 1;
		if (player.GetMana() < player.GetEffectiveManaCost(spells[index])) {
			std::cout << "  Not enough mana. Choose another action.\n";
			continue;
		}
		action.type = ActionType::CastSpell;
		action.spellIndex = index;
		std::cout << "  " << player.GetName() << " prepares to cast "
			<< spells[index].name << "!\n";
		break;
	}
	case 4: {
		Inventory& inventory = player.GetInventory();
		if (inventory.IsEmpty()) {
			std::cout << "  Your inventory is empty. Choose another action.\n";
			continue;
		}
		std::cout << "  Inventory:\n";
		inventory.ListItems();
		std::cout << "    0. Cancel\n  > ";
		int itemChoice = ReadChoice(0, static_cast<int>(inventory.Size()),
			"  Invalid item. Try again: ");
		if (itemChoice == 0) continue;
		action.type = ActionType::UseItem;
		action.itemIndex = itemChoice - 1;
		break;
	}
	case 5:
		action.type = ActionType::Inspect;
		break;
	case 6:
		action.type = ActionType::UseItem;
		action.itemIndex = -2;
		break;
	}

		return action;
	}
}

int ChooseTarget(const std::vector<Enemy>& enemies, const std::string& prompt) {
	std::vector<int> living;
	for (size_t i = 0; i < enemies.size(); ++i) {
		if (enemies[i].IsAlive()) living.push_back(static_cast<int>(i));
	}

	if (living.empty()) return -1;
	if (living.size() == 1) return living.front();

	std::cout << "\n  " << prompt << "\n";
	for (size_t i = 0; i < living.size(); ++i) {
		const int enemyIndex = living[i];
		std::cout << "    " << (i + 1) << ". [E" << (enemyIndex + 1) << "] "
			<< enemies[enemyIndex].GetName() << "\n";
	}
	std::cout << "    0. Back\n";
	std::cout << "    > ";
	const int choice = ReadChoice(0, static_cast<int>(living.size()),
		"    Invalid target. Try again: ");
	if (choice == 0) return -1;
	return living[choice - 1];
}

} // namespace CombatMenu
