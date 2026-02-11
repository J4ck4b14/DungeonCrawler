#include "Dungeon.h"
#include "entities/EnemyFactory.h"
#include "combat/CombatSystem.h"
#include "items/Item.h"
#include "utils/RNG.h"
#include <iostream>

Dungeon::Dungeon() : currentLevel_(1) {}

int Dungeon::GetCurrentLevel() const { return currentLevel_; }

Encounter Dungeon::GenerateEncounter() {
	static RNG rng;
	int roll = rng.NextInt(1, 100);

	// Probabilities shift with level — deeper floors are more dangerous
	int combatChance = 45 + currentLevel_ * 2;   // 47% at L1, up to ~65% at L10
	int chestChance = 20;
	int trapChance = 10 + currentLevel_;          // Traps increase with depth
	int restChance = 15 - currentLevel_;          // Rest becomes rarer
	if (restChance < 3) restChance = 3;

	// Normalize by checking cumulative ranges
	if (roll <= combatChance) {
		return {EncounterType::Combat, "You hear growling ahead..."};
	}
	roll -= combatChance;
	if (roll <= chestChance) {
		return {EncounterType::Chest, "You spot a dusty chest in the corner!"};
	}
	roll -= chestChance;
	if (roll <= trapChance) {
		return {EncounterType::Trap, "The ground shifts beneath your feet!"};
	}
	roll -= trapChance;
	if (roll <= restChance) {
		return {EncounterType::Rest, "You find a quiet alcove to catch your breath."};
	}
	return {EncounterType::Nothing, "An empty corridor stretches before you."};
}

void Dungeon::HandleChest(Player& player) {
	static RNG rng;
	int roll = rng.NextInt(1, 100);

	std::cout << "  You open the chest...\n";

	if (roll <= 35) {
		Item potion = MakeHealthPotion();
		if (currentLevel_ >= 5) potion = MakeLargeHealthPotion();
		player.GetInventory().AddItem(potion);
		std::cout << "  Found: " << potion.name << "!\n";
	}
	else if (roll <= 65) {
		Item potion = MakeManaPotion();
		if (currentLevel_ >= 5) potion = MakeLargeManaPotion();
		player.GetInventory().AddItem(potion);
		std::cout << "  Found: " << potion.name << "!\n";
	}
	else if (roll <= 85) {
		// Find both
		Item hp = MakeHealthPotion();
		Item mp = MakeManaPotion();
		player.GetInventory().AddItem(hp);
		player.GetInventory().AddItem(mp);
		std::cout << "  Jackpot! Found: " << hp.name << " and " << mp.name << "!\n";
	}
	else {
		std::cout << "  The chest is empty. How disappointing.\n";
	}
}

void Dungeon::HandleRest(Player& player) {
	int hpRestore = 10 + currentLevel_ * 2;
	int manaRestore = 5 + currentLevel_;
	player.Heal(hpRestore);
	player.RestoreMana(manaRestore);
	std::cout << "  You rest and recover " << hpRestore << " HP and "
		<< manaRestore << " Mana.\n";
	player.PrintStatus();
}

void Dungeon::HandleTrap(Player& player) {
	static RNG rng;
	int dmg = rng.NextInt(5, 10) + currentLevel_ * 2;
	player.ReceiveDamage(dmg);
	std::cout << "  A trap! You take " << dmg << " damage!\n";
	if (!player.IsAlive()) {
		std::cout << "  The trap proved fatal...\n";
	}
	else {
		player.PrintStatus();
	}
}

void Dungeon::HandleEncounter(Player& player, const Encounter& encounter) {
	std::cout << "\n" << encounter.description << "\n";

	switch (encounter.type) {
	case EncounterType::Combat: {
		Enemy enemy = EnemyFactory::CreateEnemy(currentLevel_);
		bool won = CombatSystem::ResolveCombat(player, enemy);
		if (!won) return; // Player died
		break;
	}
	case EncounterType::Chest:
		HandleChest(player);
		break;
	case EncounterType::Rest:
		HandleRest(player);
		break;
	case EncounterType::Trap:
		HandleTrap(player);
		break;
	case EncounterType::Nothing:
		std::cout << "  Nothing happens. You press onward.\n";
		break;
	}
}

bool Dungeon::RunFloor(Player& player) {
	static RNG rng;

	int encounters = 3 + rng.NextInt(0, 2) + currentLevel_ / 3; // 3-5 base, +1 per 3 levels

	std::cout << "\n????????????????????????????????????\n";
	std::cout << "?     DUNGEON FLOOR " << currentLevel_ << "              ?\n";
	std::cout << "????????????????????????????????????\n";
	std::cout << "You descend deeper into the dungeon...\n";
	std::cout << "(" << encounters << " rooms on this floor)\n";

	for (int i = 0; i < encounters; ++i) {
		std::cout << "\n?? Room " << (i + 1) << "/" << encounters << " ??\n";

		Encounter enc = GenerateEncounter();
		HandleEncounter(player, enc);

		if (!player.IsAlive()) return false;

		// Show status between rooms
		if (i < encounters - 1) {
			std::cout << "\nYou continue deeper...\n";
		}
	}

	std::cout << "\n??????????????????????????????????\n";
	std::cout << "  Floor " << currentLevel_ << " cleared!\n";
	std::cout << "??????????????????????????????????\n";

	// Small rest between floors
	int restHp = 15;
	int restMana = 10;
	player.Heal(restHp);
	player.RestoreMana(restMana);
	std::cout << "  Between floors you rest, recovering " << restHp << " HP and " << restMana << " Mana.\n";
	player.PrintStatus();

	currentLevel_++;
	return true;
}
