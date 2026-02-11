#include "Dungeon.h"
#include "Perception.h"
#include "entities/EnemyFactory.h"
#include "combat/CombatSystem.h"
#include "items/Item.h"
#include "utils/RNG.h"
#include "utils/Console.h"
#include <iostream>
#include <limits>
#include <algorithm>

Dungeon::Dungeon() : currentLevel_(1), gridSize_(0), playerX_(0), playerY_(0) {}

int Dungeon::GetCurrentLevel() const { return currentLevel_; }
const GameStats& Dungeon::GetStats() const { return gameStats_; }
const Bestiary& Dungeon::GetBestiary() const { return bestiary_; }

RoomContent Dungeon::GenerateRoomContent(bool isStart, bool isStaircase) {
	if (isStart) return RoomContent::Empty;
	if (isStaircase) return RoomContent::Staircase;

	static RNG rng;
	int roll = rng.NextInt(1, 100);

	int combatChance = 40 + currentLevel_ * 2;
	int chestChance = 15;
	int trapChance = 8 + currentLevel_;
	int restChance = 12 - currentLevel_;
	if (restChance < 2) restChance = 2;

	if (roll <= combatChance) return RoomContent::Combat;
	roll -= combatChance;
	if (roll <= chestChance) return RoomContent::Chest;
	roll -= chestChance;
	if (roll <= trapChance) return RoomContent::Trap;
	roll -= trapChance;
	if (roll <= restChance) return RoomContent::Rest;
	return RoomContent::Empty;
}

void Dungeon::GenerateFloor() {
	static RNG rng;

	gridSize_ = 4 + (currentLevel_ - 1) / 3;
	if (gridSize_ > 8) gridSize_ = 8;

	grid_.clear();
	grid_.resize(gridSize_, std::vector<Room>(gridSize_));

	int startEdge = rng.NextInt(0, 3);
	switch (startEdge) {
	case 0: playerX_ = rng.NextInt(0, gridSize_ - 1); playerY_ = 0; break;
	case 1: playerX_ = gridSize_ - 1; playerY_ = rng.NextInt(0, gridSize_ - 1); break;
	case 2: playerX_ = rng.NextInt(0, gridSize_ - 1); playerY_ = gridSize_ - 1; break;
	case 3: playerX_ = 0; playerY_ = rng.NextInt(0, gridSize_ - 1); break;
	}

	int stairX, stairY;
	do {
		stairX = rng.NextInt(0, gridSize_ - 1);
		stairY = rng.NextInt(0, gridSize_ - 1);
	} while (std::abs(stairX - playerX_) + std::abs(stairY - playerY_) < gridSize_);

	for (int y = 0; y < gridSize_; ++y) {
		for (int x = 0; x < gridSize_; ++x) {
			Room& room = grid_[y][x];
			room.x = x;
			room.y = y;
			bool isStart = (x == playerX_ && y == playerY_);
			bool isStair = (x == stairX && y == stairY);
			room.content = GenerateRoomContent(isStart, isStair);
		}
	}

	// Guaranteed path from start to staircase
	{
		int cx = playerX_, cy = playerY_;
		while (cx != stairX || cy != stairY) {
			Direction dir;
			if (cx != stairX && cy != stairY) {
				if (rng.Chance(0.5f))
					dir = (stairX > cx) ? Direction::East : Direction::West;
				else
					dir = (stairY > cy) ? Direction::South : Direction::North;
			}
			else if (cx != stairX)
				dir = (stairX > cx) ? Direction::East : Direction::West;
			else
				dir = (stairY > cy) ? Direction::South : Direction::North;

			grid_[cy][cx].SetExit(dir, true);
			switch (dir) {
			case Direction::North: cy--; break;
			case Direction::South: cy++; break;
			case Direction::East:  cx++; break;
			case Direction::West:  cx--; break;
			}
			grid_[cy][cx].SetExit(OppositeDirection(dir), true);
		}
	}

	// Random extra connections
	for (int y = 0; y < gridSize_; ++y) {
		for (int x = 0; x < gridSize_; ++x) {
			if (x + 1 < gridSize_ && !grid_[y][x].HasExit(Direction::East)) {
				if (rng.Chance(0.35f)) {
					grid_[y][x].SetExit(Direction::East, true);
					grid_[y][x + 1].SetExit(Direction::West, true);
				}
			}
			if (y + 1 < gridSize_ && !grid_[y][x].HasExit(Direction::South)) {
				if (rng.Chance(0.35f)) {
					grid_[y][x].SetExit(Direction::South, true);
					grid_[y + 1][x].SetExit(Direction::North, true);
				}
			}
		}
	}

	grid_[playerY_][playerX_].visited = true;
	grid_[playerY_][playerX_].contentResolved = true;
}

void Dungeon::PrintMap() const {
	std::cout << "\n  MAP (Level " << currentLevel_ << "):\n";
	std::cout << "  Legend: @ = You  ? = Unvisited  V = Staircase\n\n";

	for (int y = 0; y < gridSize_; ++y) {
		std::cout << "  ";
		for (int x = 0; x < gridSize_; ++x) {
			std::cout << "+";
			if (grid_[y][x].HasExit(Direction::North) && y > 0) {
				if (grid_[y][x].visited || grid_[y - 1][x].visited)
					std::cout << "  ";
				else
					std::cout << "--";
			}
			else {
				std::cout << "--";
			}
		}
		std::cout << "+\n";

		std::cout << "  ";
		for (int x = 0; x < gridSize_; ++x) {
			if (grid_[y][x].HasExit(Direction::West) && x > 0) {
				if (grid_[y][x].visited || grid_[y][x - 1].visited)
					std::cout << " ";
				else
					std::cout << "|";
			}
			else {
				std::cout << "|";
			}

			if (x == playerX_ && y == playerY_)
				std::cout << "@ ";
			else if (grid_[y][x].visited) {
				if (grid_[y][x].content == RoomContent::Staircase && !grid_[y][x].contentResolved)
					std::cout << "V ";
				else
					std::cout << "  ";
			}
			else
				std::cout << "? ";
		}
		std::cout << "|\n";
	}

	std::cout << "  ";
	for (int x = 0; x < gridSize_; ++x)
		std::cout << "+--";
	std::cout << "+\n\n";
}

void Dungeon::HandleCombat(Player& player) {
	Enemy enemy = EnemyFactory::CreateEnemy(currentLevel_);
	CombatSystem::ResolveCombat(player, enemy, seenEnemyTypes_, gameStats_, bestiary_);
}

void Dungeon::HandleChest(Player& player) {
	static RNG rng;
	int roll = rng.NextInt(1, 100);

	gameStats_.chestsOpened++;
	Console::PrintSlow("  You find a chest and pry it open...");

	if (roll <= 30) {
		Item potion = (currentLevel_ >= 5) ? MakeLargeHealthPotion() : MakeHealthPotion();
		player.GetInventory().AddItem(potion);
		Console::PrintSlow("  Found: " + potion.name + "!");
	}
	else if (roll <= 55) {
		Item potion = (currentLevel_ >= 5) ? MakeLargeManaPotion() : MakeManaPotion();
		player.GetInventory().AddItem(potion);
		Console::PrintSlow("  Found: " + potion.name + "!");
	}
	else if (roll <= 80) {
		Item hp = MakeHealthPotion();
		Item mp = MakeManaPotion();
		player.GetInventory().AddItem(hp);
		player.GetInventory().AddItem(mp);
		Console::PrintSlow("  Jackpot! Found: " + hp.name + " and " + mp.name + "!");
	}
	else {
		Console::PrintSlow("  The chest is empty. How disappointing.");
	}
}

void Dungeon::HandleRest(Player& player) {
	Console::PrintSlow("  You find a quiet alcove. What do you do?");
	std::cout << "\n";
	std::cout << "    1. Rest (restore HP and Mana)\n";

	bool canTrain = player.CanTrain();
	if (canTrain)
		std::cout << "    2. Train (permanent +1 stat, " << player.GetTrainingPoints() << "/3 used)\n";
	else
		std::cout << "    2. Train (MAXED OUT - 3/3)\n";

	std::cout << "    3. Sharpen weapon (bonus physical damage for next few attacks)\n";
	std::cout << "    4. Study the arcane (bonus spell damage for next few casts)\n";
	std::cout << "  > ";

	int choice = 0;
	std::cin >> choice;
	while (std::cin.fail() || choice < 1 || choice > 4) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "  Invalid. Enter 1-4: ";
		std::cin >> choice;
	}

	static RNG rng;

	switch (choice) {
	case 1: {
		// Rest: restore HP and Mana
		int hpRestore = 5 + currentLevel_;
		int manaRestore = 3 + currentLevel_ / 2;
		player.Heal(hpRestore);
		player.RestoreMana(manaRestore);
		Console::PrintSlow("  You rest and recover.");
		Console::PrintSlow("  Restored " + std::to_string(hpRestore) + " HP and "
			+ std::to_string(manaRestore) + " Mana.");
		break;
	}
	case 2: {
		// Train: permanent stat boost (capped)
		if (!canTrain) {
			Console::PrintSlow("  You've trained as much as your body allows (3/3).");
			Console::PrintSlow("  You rest instead.");
			int hpRestore = 5 + currentLevel_;
			int manaRestore = 3 + currentLevel_ / 2;
			player.Heal(hpRestore);
			player.RestoreMana(manaRestore);
			Console::PrintSlow("  Restored " + std::to_string(hpRestore) + " HP and "
				+ std::to_string(manaRestore) + " Mana.");
			break;
		}
		std::cout << "  Choose a stat to train:\n";
		std::cout << "    1. Health (+5 max HP)\n";
		std::cout << "    2. Attack (+1 ATK)\n";
		std::cout << "    3. Speed  (+1 SPD)\n";
		std::cout << "    4. Intelligence (+1 INT, +3 max Mana)\n";
		std::cout << "  > ";
		int stat = 0;
		std::cin >> stat;
		while (std::cin.fail() || stat < 1 || stat > 4) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "  Invalid. Enter 1-4: ";
			std::cin >> stat;
		}
		player.TrainStat(stat);
		Console::PrintSlow("  You skip rest to push yourself. Training complete.");
		player.PrintStatus();
		break;
	}
	case 3: {
		// Sharpen weapon: temporary physical attack buff
		int bonus = rng.NextInt(2, 5);
		int hits = rng.NextInt(3, 7);
		player.ApplyAttackBuff(bonus, hits, false);
		Console::PrintSlow("  You sharpen your weapon on the stone walls.");
		Console::PrintSlow("  +" + std::to_string(bonus)
			+ " physical damage for the next " + std::to_string(hits) + " attacks!");
		break;
	}
	case 4: {
		// Study the arcane: temporary spell damage buff
		int bonus = rng.NextInt(2, 5);
		int hits = rng.NextInt(3, 7);
		player.ApplyAttackBuff(bonus, hits, true);
		Console::PrintSlow("  You meditate and focus your magical energy.");
		Console::PrintSlow("  +" + std::to_string(bonus)
			+ " spell damage for the next " + std::to_string(hits) + " casts!");
		break;
	}
	}
}

void Dungeon::HandleTrap(Player& player, Room& room) {
	static RNG rng;

	bool warned = false;
	for (int d = 0; d < 4; ++d) {
		Direction checkDir = static_cast<Direction>(d);
		int adjX = room.x, adjY = room.y;
		switch (checkDir) {
		case Direction::North: adjY--; break;
		case Direction::South: adjY++; break;
		case Direction::East:  adjX++; break;
		case Direction::West:  adjX--; break;
		}
		if (adjX < 0 || adjX >= gridSize_ || adjY < 0 || adjY >= gridSize_) continue;

		const Room& adjRoom = grid_[adjY][adjX];
		for (const auto& hint : adjRoom.hints) {
			if (hint.revealsContent && hint.revealedContent == RoomContent::Trap &&
				hint.direction == OppositeDirection(checkDir)) {
				warned = true;
				break;
			}
		}
		if (warned) break;
	}

	if (warned) {
		gameStats_.trapsAvoided++;
		Console::PrintSlow("  You recall the trap you sensed earlier and carefully step around it!");
		Console::PrintSlow("  The trap is disarmed - your perception saved you.");
	}
	else {
		gameStats_.trapsTriggered++;
		int dmg = rng.NextInt(3, 6) + currentLevel_;
		player.ReceiveDamage(dmg);
		gameStats_.totalDamageTaken += dmg;
		Console::PrintSlow("  A trap springs! You take " + std::to_string(dmg) + " damage!");
		if (!player.IsAlive()) {
			Console::PrintSlow("  The trap proved fatal...");
		}
		else {
			player.PrintStatus();
		}
	}
}

void Dungeon::AwardExplorationXP(Player& player) {
	int xp = 2 + currentLevel_;
	player.GainXP(xp);
}

void Dungeon::HandleRoomContent(Player& player, Room& room) {
	if (room.contentResolved) {
		std::cout << "  This room has already been cleared.\n";
		return;
	}

	switch (room.content) {
	case RoomContent::Combat:
		Console::PrintSlow("  Something stirs in the shadows...");
		Console::WaitForEnter();
		HandleCombat(player);
		break;
	case RoomContent::Chest:
		HandleChest(player);
		break;
	case RoomContent::Rest:
		HandleRest(player);
		break;
	case RoomContent::Trap:
		HandleTrap(player, room);
		break;
	case RoomContent::Staircase:
		Console::PrintSlow("  You find a staircase spiraling downward into darkness.");
		return;
	case RoomContent::Empty:
		Console::PrintSlow("  The room is empty. Dust motes drift in the stale air.");
		break;
	}

	room.contentResolved = true;
	if (player.IsAlive()) {
		gameStats_.roomsExplored++;
		AwardExplorationXP(player);
	}
}

void Dungeon::EnterRoom(Player& player) {
	Room& room = grid_[playerY_][playerX_];
	room.visited = true;

	Console::WaitForEnter();
	Console::Clear();
	Console::PrintSlow("\n-- Room (" + std::to_string(room.x) + ", "
		+ std::to_string(room.y) + ") --");

	HandleRoomContent(player, room);
}

bool Dungeon::PromptMovement(Player& player) {
	while (player.IsAlive()) {
		Room& current = grid_[playerY_][playerX_];

		PrintMap();
		player.PrintStatus();

		std::cout << "\n  What do you do?\n";
		int optNum = 1;

		struct MoveOption { Direction dir; int num; };
		std::vector<MoveOption> moves;

		for (int d = 0; d < 4; d++) {
			Direction dir = static_cast<Direction>(d);
			if (current.HasExit(dir)) {
				int nx = playerX_, ny = playerY_;
				switch (dir) {
				case Direction::North: ny--; break;
				case Direction::South: ny++; break;
				case Direction::East:  nx++; break;
				case Direction::West:  nx--; break;
				}
				if (nx >= 0 && nx < gridSize_ && ny >= 0 && ny < gridSize_) {
					const Room& adj = grid_[ny][nx];
					std::string label = std::string("Move ") + DirectionName(dir);
					if (adj.visited) label += " (visited)";
					else label += " (unexplored)";
					std::cout << "    " << optNum << ". " << label << "\n";
					moves.push_back({dir, optNum});
					optNum++;
				}
			}
		}

		int perceiveOpt = 0;
		if (!current.perceptionUsed) {
			perceiveOpt = optNum;
			std::cout << "    " << optNum << ". Perception check (survey surroundings)\n";
			optNum++;
		}

		int inventoryOpt = 0;
		if (!player.GetInventory().IsEmpty()) {
			inventoryOpt = optNum;
			std::cout << "    " << optNum << ". Use item\n";
			optNum++;
		}

		int descOpt = 0;
		if (current.content == RoomContent::Staircase && !current.contentResolved) {
			descOpt = optNum;
			std::cout << "    " << optNum << ". Descend staircase (next floor)\n";
			optNum++;
		}

		// Bestiary is always available
		int bestiaryOpt = optNum;
		std::cout << "    " << optNum << ". Bestiary (" << bestiary_.GetEntryCount() << " entries)\n";
		optNum++;

		std::cout << "  > ";
		int choice = 0;
		std::cin >> choice;

		while (std::cin.fail() || choice < 1 || choice >= optNum) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "  Invalid. Enter 1-" << (optNum - 1) << ": ";
			std::cin >> choice;
		}

		for (const auto& m : moves) {
			if (choice == m.num) {
				switch (m.dir) {
				case Direction::North: playerY_--; break;
				case Direction::South: playerY_++; break;
				case Direction::East:  playerX_++; break;
				case Direction::West:  playerX_--; break;
				}
				Console::PrintSlow("\n  You move " + std::string(DirectionName(m.dir)) + "...");
				EnterRoom(player);
				if (!player.IsAlive()) return false;
				goto continue_loop;
			}
		}

		if (perceiveOpt > 0 && choice == perceiveOpt) {
			Perception::PerceiveFromRoom(current, grid_, gridSize_, player);
			continue;
		}

		if (inventoryOpt > 0 && choice == inventoryOpt) {
			std::cout << "  Inventory:\n";
			player.GetInventory().ListItems();
			std::cout << "    0. Cancel\n  > ";
			int itemChoice = 0;
			std::cin >> itemChoice;
			if (itemChoice >= 1 && itemChoice <= static_cast<int>(player.GetInventory().Size())) {
				int hp = player.GetHP();
				int mana = player.GetMana();
				player.GetInventory().UseItem(
					itemChoice - 1, hp, player.GetMaxHP(), mana, player.GetMaxMana());
				if (hp > player.GetHP()) player.Heal(hp - player.GetHP());
				if (mana > player.GetMana()) player.RestoreMana(mana - player.GetMana());
			}
			continue;
		}

		// After inventory check, before descend check:
		if (choice == bestiaryOpt) {
			bestiary_.Print();
			continue;
		}

		if (descOpt > 0 && choice == descOpt) {
			current.contentResolved = true;
			return true;
		}

		continue_loop:;
	}

	return false;
}

bool Dungeon::RunFloor(Player& player) {
	GenerateFloor();
	seenEnemyTypes_.clear();

	Console::WaitForEnter();
	Console::Clear();
	Console::PrintSlow("\n+======================================+");
	std::string floorLine = "|       DUNGEON FLOOR " + std::to_string(currentLevel_);
	while (floorLine.size() < 39) floorLine += " ";
	floorLine += "|";
	Console::PrintSlow(floorLine);
	Console::PrintSlow("+======================================+");
	Console::PrintSlow(std::string("You ") + (currentLevel_ == 1 ? "enter" : "descend to")
		+ " floor " + std::to_string(currentLevel_) + " of the dungeon.");
	Console::PrintSlow("The floor is a " + std::to_string(gridSize_) + "x"
		+ std::to_string(gridSize_) + " grid of rooms.");
	Console::PrintSlow("Find the staircase to proceed deeper!");

	Console::PrintSlow("\n-- Starting Room --");
	Console::PrintSlow("  You stand at the entrance. The air is cold and damp.");

	bool descended = PromptMovement(player);

	if (!player.IsAlive()) return false;

	if (descended) {
		Console::WaitForEnter();
		Console::Clear();
		Console::PrintSlow("\n==================================");
		Console::PrintSlow("  Floor " + std::to_string(currentLevel_) + " cleared!");
		gameStats_.floorsCleared++;

		int visited = 0;
		int total = gridSize_ * gridSize_;
		for (int y = 0; y < gridSize_; ++y)
			for (int x = 0; x < gridSize_; ++x)
				if (grid_[y][x].visited) visited++;

		Console::PrintSlow("  Rooms explored: " + std::to_string(visited)
			+ "/" + std::to_string(total));
		int bonusXP = visited * 2;
		std::cout << "  Exploration bonus: ";
		player.GainXP(bonusXP);

		Console::PrintSlow("==================================");

		int restHp = 5 + currentLevel_;
		int restMana = 3 + currentLevel_ / 2;
		player.Heal(restHp);
		player.RestoreMana(restMana);
		Console::PrintSlow("  Between floors you rest, recovering " + std::to_string(restHp)
			+ " HP and " + std::to_string(restMana) + " Mana.");
		player.PrintStatus();

		currentLevel_++;
		return true;
	}

	return false;
}
