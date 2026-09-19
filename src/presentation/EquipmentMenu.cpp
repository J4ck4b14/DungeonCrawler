#include "EquipmentMenu.h"

#include "entities/Player.h"

#include <iostream>
#include <limits>
#include <string>
#include <utility>

namespace {
int ReadChoice() {
	int choice = 0;
	while (true) {
		std::cout << "  > ";
		std::cin >> choice;
		if (!std::cin.fail() && choice >= 1 && choice <= 2) return choice;
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "  Invalid. Enter 1 or 2.\n";
	}
}

std::string DescribeEffects(const Apparel& apparel) {
	const ApparelEffects effects = apparel.GetDerivedEffects();
	std::string text;
	if (effects.armor > 0) text += "Armor +" + std::to_string(effects.armor);
	if (effects.spellPower > 0) text += (text.empty() ? "" : ", ")
		+ std::string("Spell Power +") + std::to_string(effects.spellPower);
	if (effects.maxMana > 0) text += (text.empty() ? "" : ", ")
		+ std::string("Max Mana +") + std::to_string(effects.maxMana);
	if (effects.speed > 0) text += (text.empty() ? "" : ", ")
		+ std::string("Speed +") + std::to_string(effects.speed);
	return text;
}
} // namespace

namespace EquipmentMenu {

bool Offer(Player& player, GeneratedEquipment equipment) {
	if (Weapon* found = std::get_if<Weapon>(&equipment)) {
		std::cout << "\n  NEW:     " << found->GetDisplayName()
			<< " | Damage " << found->CalculateDamage(player.GetStrength(),
				player.GetSpeed(), player.GetIntelligence()) << "\n";
		const auto& current = player.GetEquipment().GetWeapon();
		if (current) {
			std::cout << "  CURRENT: " << current->GetDisplayName()
				<< " | Damage " << current->CalculateDamage(player.GetStrength(),
					player.GetSpeed(), player.GetIntelligence()) << "\n";
		} else {
			std::cout << "  CURRENT: Empty\n";
		}
		std::cout << "    1. Equip new\n    2. Leave it\n";
		if (ReadChoice() == 2) return false;
		const bool replaced = current.has_value();
		player.EquipWeapon(std::move(*found));
		if (replaced) std::cout << "  The previous weapon is lost.\n";
		return true;
	}

	Apparel& found = std::get<Apparel>(equipment);
	std::cout << "\n  NEW:     " << found.GetDisplayName()
		<< " | " << DescribeEffects(found) << "\n";
	const auto& current = player.GetEquipment().GetApparel(found.GetSlot());
	if (current) {
		std::cout << "  CURRENT: " << current->GetDisplayName()
			<< " | " << DescribeEffects(*current) << "\n";
	} else {
		std::cout << "  CURRENT: Empty\n";
	}
	std::cout << "    1. Equip new\n    2. Leave it\n";
	if (ReadChoice() == 2) return false;
	const bool replaced = current.has_value();
	player.EquipApparel(std::move(found));
	if (replaced) std::cout << "  The previous apparel in that slot is lost.\n";
	return true;
}

void PrintLoadout(const Player& player) {
	const auto& weapon = player.GetEquipment().GetWeapon();
	if (weapon) {
		std::cout << "  Weapon: " << weapon->GetDisplayName()
			<< " | Damage " << player.GetWeaponDamage() << "\n";
	}
}

} // namespace EquipmentMenu
