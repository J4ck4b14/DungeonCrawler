#include "Dungeon.h"
#include "Perception.h"
#include "entities/EnemyFactory.h"
#include "combat/CombatSystem.h"
#include "items/Item.h"
#include "utils/RNG.h"
#include "utils/Console.h"
#include "core/DevMode.h"
#include "core/Relic.h"
#include "combat/Spell.h"
#include "Room.h"
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

	float enemyScale = 1.0f;
	float trapMul = 1.0f;
	if (DevMode::IsEnabled()) {
		enemyScale = DevMode::GetEnemyScale();
		trapMul = DevMode::GetTrapMultiplier();
	}

	int combatChance = static_cast<int>((40 + currentLevel_ * 2) * enemyScale);
	int chestChance = static_cast<int>(15 * (1.0f / enemyScale)); // fewer chests if enemies scaled up
	int trapChance = static_cast<int>((8 + currentLevel_) * trapMul);
	int restChance = static_cast<int>((12 - currentLevel_) * (1.0f / trapMul));
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

	// Random hidden / breakable connections (behind a brittle wall)
	for (int y = 0; y < gridSize_; ++y) {
		for (int x = 0; x < gridSize_; ++x) {
			// East
			if (x + 1 < gridSize_ && !grid_[y][x].HasExit(Direction::East)
				&& !grid_[y][x + 1].HasExit(Direction::West)) {
				// Small chance to create hidden breakable wall between these rooms
				if (rng.Chance(0.12f)) {
					// Choose a material and assign a hidden HP + optional elemental weakness.
					int baseHp = rng.NextInt(6, 14) + (currentLevel_ / 2);
					WallMaterial mat;
					SpellElement weakness = SpellElement::Arcane;
					if (baseHp <= 8) {
						mat = WallMaterial::Wood;
						weakness = SpellElement::Fire;
					}
					else if (baseHp <= 14) {
						mat = WallMaterial::Stone;
						weakness = SpellElement::Arcane;
					}
					else {
						mat = WallMaterial::Strange;
						weakness = (rng.Chance(0.5f)) ? SpellElement::Arcane : SpellElement::Shadow;
						// Strange materials scale harder with depth
						baseHp += currentLevel_;
					}
					grid_[y][x].SetHiddenExit(Direction::East, true, baseHp);
					grid_[y][x].hiddenMaterial[static_cast<int>(Direction::East)] = mat;
					grid_[y][x].hiddenWeakness[static_cast<int>(Direction::East)] = weakness;

					grid_[y][x + 1].SetHiddenExit(Direction::West, true, baseHp);
					grid_[y][x + 1].hiddenMaterial[static_cast<int>(Direction::West)] = mat;
					grid_[y][x + 1].hiddenWeakness[static_cast<int>(Direction::West)] = weakness;
				}
			}
			// South
			if (y + 1 < gridSize_ && !grid_[y][x].HasExit(Direction::South)
				&& !grid_[y + 1][x].HasExit(Direction::North)) {
				if (rng.Chance(0.12f)) {
					int baseHp = rng.NextInt(6, 14) + (currentLevel_ / 2);
					WallMaterial mat;
					SpellElement weakness = SpellElement::Arcane;
					if (baseHp <= 8) {
						mat = WallMaterial::Wood;
						weakness = SpellElement::Fire;
					}
					else if (baseHp <= 14) {
						mat = WallMaterial::Stone;
						weakness = SpellElement::Arcane;
					}
					else {
						mat = WallMaterial::Strange;
						weakness = (rng.Chance(0.5f)) ? SpellElement::Arcane : SpellElement::Shadow;
						baseHp += currentLevel_;
					}
					grid_[y][x].SetHiddenExit(Direction::South, true, baseHp);
					grid_[y][x].hiddenMaterial[static_cast<int>(Direction::South)] = mat;
					grid_[y][x].hiddenWeakness[static_cast<int>(Direction::South)] = weakness;

					grid_[y + 1][x].SetHiddenExit(Direction::North, true, baseHp);
					grid_[y + 1][x].hiddenMaterial[static_cast<int>(Direction::North)] = mat;
					grid_[y + 1][x].hiddenWeakness[static_cast<int>(Direction::North)] = weakness;
				}
			}
		}
	}

	grid_[playerY_][playerX_].visited = true;
	grid_[playerY_][playerX_].contentResolved = true;
}

void Dungeon::PrintMap() const {
	std::cout << "\n  MAP (Level " << currentLevel_ << "):\n";
	std::cout << "  Legend: @ = You  ? = Adjacent (unknown)  V = Staircase\n\n";

	// Helpers
	auto isVisible = [&](int x, int y) -> bool {
		if (DevMode::IsEnabled() && DevMode::RevealMapEnabled()) return true;
		return grid_[y][x].visited;
	};
	auto isAdjacentToVisited = [&](int x, int y) -> bool {
		const int dx[4] = { 0, 1, 0, -1 };
		const int dy[4] = { -1, 0, 1, 0 };
		for (int i = 0; i < 4; ++i) {
			int nx = x + dx[i], ny = y + dy[i];
			if (nx >= 0 && nx < gridSize_ && ny >= 0 && ny < gridSize_) {
				if (grid_[ny][nx].visited) return true;
			}
		}
		return false;
	};

	// Compute vertical cropping: show only explored rows plus a 1-row margin.
	int minY = gridSize_, maxY = -1;
	int minX = gridSize_, maxX = -1;
	for (int y = 0; y < gridSize_; ++y) {
		for (int x = 0; x < gridSize_; ++x) {
			if (grid_[y][x].visited) {
				minY = std::min(minY, y);
				maxY = std::max(maxY, y);
				minX = std::min(minX, x);
				maxX = std::max(maxX, x);
			}
		}
	}

	// Nothing visited yet -> small window around player
	if (maxY == -1) {
		minY = std::max(0, playerY_ - 1);
		maxY = std::min(gridSize_ - 1, playerY_ + 1);
		minX = std::max(0, playerX_ - 1);
		maxX = std::min(gridSize_ - 1, playerX_ + 1);
	}
	// Add 1-cell margin, clamp
	minY = std::max(0, minY - 1);
	maxY = std::min(gridSize_ - 1, maxY + 1);
	minX = std::max(0, minX - 1);
	maxX = std::min(gridSize_ - 1, maxX + 1);

	// Iterate rows in cropped range
	for (int y = minY; y <= maxY; ++y) {
		// Top border line for this row
		std::cout << "  ";
		bool anyPlus = false;
		std::string topLine;
		for (int x = minX; x <= maxX; ++x) {
			bool vis = isVisible(x, y);
			bool visAbove = (y > 0 && isVisible(x, y - 1));

			if (vis || visAbove) {
				anyPlus = true;
				topLine += "+";
				// Only show a clear connector if both rooms (this and above) are visible
				if (grid_[y][x].HasExit(Direction::North) && vis && visAbove)
					topLine += "  ";
				else
					topLine += "--";
			}
			else {
				// Hide completely (preserve spacing)
				topLine += "   ";
			}
		}
		topLine += anyPlus ? "+" : " ";
		std::cout << topLine << "\n";

		// Middle line: verticals + room contents
		std::cout << "  ";
		for (int x = minX; x <= maxX; ++x) {
			bool vis = isVisible(x, y);
			bool visLeft = (x > 0 && isVisible(x - 1, y));

			// Vertical border: only render when either side is visible; actual opening shown only if both visible
			if (vis || visLeft) {
				if (grid_[y][x].HasExit(Direction::West) && vis && visLeft)
					std::cout << " ";
				else
					std::cout << "|";
			}
			else {
				std::cout << " ";
			}

			// Room content / marker
			if (x == playerX_ && y == playerY_)
				std::cout << "@ ";
			else if (vis) {
				if (grid_[y][x].content == RoomContent::Staircase && !grid_[y][x].contentResolved)
					std::cout << "V ";
				else
					std::cout << "  ";
			}
			else if (isAdjacentToVisited(x, y)) {
				// Adjacent to a visited room => show an unknown marker
				std::cout << "? ";
			}
			else {
				// Completely hidden
				std::cout << "  ";
			}
		}

		// Rightmost border for the row
		bool anyRight = isVisible(maxX, y) || (maxX > minX && isVisible(maxX - 1, y));
		std::cout << (anyRight ? "|\n" : " \n");
	}

	// Bottom border for the cropped area
	std::cout << "  ";
	for (int x = minX; x <= maxX; ++x) {
		if (isVisible(x, maxY)) {
			std::cout << "+--";
		}
		else {
			std::cout << "   ";
		}
	}
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

	HandleRoomContent(player, room);
}

bool Dungeon::PromptMovement(Player& player) {
	while (player.IsAlive()) {
		Room& current = grid_[playerY_][playerX_];

		PrintMap();
		player.PrintStatus();

		std::cout << "\n  What do you do?\n";
		int optNum = 1;

		struct MoveOption { Direction dir; int num; bool isHidden; };
		std::vector<MoveOption> moves;

		// Collect normal movement / hidden-break options
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
					moves.push_back({dir, optNum, false});
					optNum++;
				}
			}
			else if (current.HasHiddenExit(dir)) {
				// Hidden/breakable wall option (explicit)
				int nx = playerX_, ny = playerY_;
				switch (dir) {
				case Direction::North: ny--; break;
				case Direction::South: ny++; break;
				case Direction::East:  nx++; break;
				case Direction::West:  nx--; break;
				}
				if (nx >= 0 && nx < gridSize_ && ny >= 0 && ny < gridSize_) {
					std::string label = std::string("Attempt to break wall ") + DirectionName(dir)
						+ " (strength check)";
					std::cout << "    " << optNum << ". " << label << "\n";
					moves.push_back({dir, optNum, true});
					optNum++;
				}
			}
		}

		// Attack wall option: allow attacking any adjacent wall (hidden or solid)
		bool anyWall = false;
		for (int d = 0; d < 4; ++d) {
			Direction dir = static_cast<Direction>(d);
			int nx = playerX_, ny = playerY_;
			switch (dir) {
			case Direction::North: ny--; break;
			case Direction::South: ny++; break;
			case Direction::East:  nx++; break;
			case Direction::West:  nx--; break;
			}
			if (nx < 0 || nx >= gridSize_ || ny < 0 || ny >= gridSize_) continue;
			// treat as a wall if there's no open exit (either hidden or plain)
			if (!current.HasExit(dir)) { anyWall = true; break; }
		}
		int attackWallOpt = 0;
		if (anyWall) {
			attackWallOpt = optNum;
			std::cout << "    " << optNum << ". Attack a wall\n";
			optNum++;
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

		static RNG rng;

		// Helper: ensure a wall entry exists between current and direction (creates one if necessary)
		auto EnsureWallExists = [&](Room& room, Direction dir) -> void {
			int idx = static_cast<int>(dir);
			if (room.HasHiddenExit(dir)) return; // already present
			// create implicit hidden wall with reasonable defaults (scaled by depth)
			int baseHp = rng.NextInt(6, 12) + (currentLevel_ / 2);
			WallMaterial mat;
			SpellElement weakness = SpellElement::Arcane;
			if (baseHp <= 8) { mat = WallMaterial::Wood; weakness = SpellElement::Fire; }
			else if (baseHp <= 14) { mat = WallMaterial::Stone; weakness = SpellElement::Arcane; }
			else { mat = WallMaterial::Strange; weakness = (rng.Chance(0.5f)) ? SpellElement::Arcane : SpellElement::Shadow; baseHp += currentLevel_; }
			room.SetHiddenExit(dir, true, baseHp);
			room.hiddenMaterial[idx] = mat;
			room.hiddenWeakness[idx] = weakness;
			// Also set on adjacent room for symmetry
			int ax = playerX_, ay = playerY_;
			switch (dir) { case Direction::North: ay--; break; case Direction::South: ay++; break; case Direction::East: ax++; break; case Direction::West: ax--; break; }
			if (ax >= 0 && ax < gridSize_ && ay >= 0 && ay < gridSize_) {
				grid_[ay][ax].SetHiddenExit(OppositeDirection(dir), true, baseHp);
				grid_[ay][ax].hiddenMaterial[static_cast<int>(OppositeDirection(dir))] = mat;
				grid_[ay][ax].hiddenWeakness[static_cast<int>(OppositeDirection(dir))] = weakness;
			}
		};

		// Wall attack handler: does physical or magical attempts, mirrors earlier logic
		auto HandleWallAttack = [&](Direction dir) -> bool {
			// Walls on the map edge are the dungeon's bedrock: unbreakable.
			// (Also prevents breaking through and walking off the grid.)
			{
				int tx = playerX_, ty = playerY_;
				switch (dir) {
				case Direction::North: ty--; break;
				case Direction::South: ty++; break;
				case Direction::East:  tx++; break;
				case Direction::West:  tx--; break;
				}
				if (tx < 0 || tx >= gridSize_ || ty < 0 || ty >= gridSize_) {
					Console::PrintSlow("\n  You strike the " + std::string(DirectionName(dir))
						+ " wall... solid bedrock. The dungeon itself. Unbreakable.");
					return true; // Not fatal, just futile
				}
			}
			// prepare wall (create if missing)
			if (!current.HasHiddenExit(dir)) {
				EnsureWallExists(current, dir);
			}
			int wallHp = current.GetHiddenToughness(dir);
			auto mat = current.hiddenMaterial[static_cast<int>(dir)];
			auto weakness = current.hiddenWeakness[static_cast<int>(dir)];

			Console::PrintSlow("\n  A solid barrier blocks the " + std::string(DirectionName(dir)) + ".");

			// choose method
			std::cout << "    1. Strike it physically\n";
			std::cout << "    2. Cast a spell at it\n";
			std::cout << "    0. Cancel\n";
			std::cout << "  > ";
			int sub = -1;
			std::cin >> sub;
			while (std::cin.fail() || sub < 0 || sub > 2) {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				std::cout << "  Invalid. Enter 0-2: ";
				std::cin >> sub;
			}
			if (sub == 0) return false;

			int ax = playerX_, ay = playerY_;
			switch (dir) { case Direction::North: ay--; break; case Direction::South: ay++; break; case Direction::East: ax++; break; case Direction::West: ax--; break; }

			if (sub == 1) {
				int rawRoll = rng.NextInt(1, 20);
				int phys = player.GetATK() + rng.NextInt(1, 4) + (rawRoll == 20 ? 4 : 0);
				Console::PrintSlow("\n  You strike the barrier with all your might...");
				if (rawRoll == 20) Console::PrintSlow("  A perfect hit! You land a heavy blow!");
				if (rawRoll == 1) Console::PrintSlow("  Your blow slips and hurts you!");

				int effective = phys;
				if (mat == WallMaterial::Strange) effective = phys / 4;

				if (effective >= wallHp) {
					// break both sides
					current.SetHiddenExit(dir, false, 0);
					current.SetExit(dir, true);
					if (ax >= 0 && ax < gridSize_ && ay >= 0 && ay < gridSize_) {
						grid_[ay][ax].SetHiddenExit(OppositeDirection(dir), false, 0);
						grid_[ay][ax].SetExit(OppositeDirection(dir), true);
					}
					Console::PrintSlow("  Your strike breaks the barrier! A passage opens.");
					// move player into adjacent room
					switch (dir) { case Direction::North: playerY_--; break; case Direction::South: playerY_++; break; case Direction::East: playerX_++; break; case Direction::West: playerX_--; break; }
					EnterRoom(player);
					return !player.IsAlive() ? false : true;
				}
				else {
					int lost = std::max(1, effective / 2);
					current.hiddenToughness[static_cast<int>(dir)] = std::max(1, wallHp - lost);
					Console::PrintSlow("  The blow chips at the barrier, but it holds.");
					if (rawRoll == 1 || rng.Chance(0.18f)) {
						int dmg = rng.NextInt(1, 3) + (currentLevel_ / 2);
						player.ReceiveDamage(dmg);
						gameStats_.totalDamageTaken += dmg;
						Console::PrintSlow("  You strain yourself and take " + std::to_string(dmg) + " damage!");
						if (!player.IsAlive()) {
							Console::PrintSlow("  The effort proved fatal...");
							return false;
						}
						else {
							player.PrintStatus();
						}
					}
					return true;
				}
			}

			// Cast spell at wall
			const auto& spells = player.GetKnownSpells();
			if (spells.empty()) {
				Console::PrintSlow("  You don't know spells to try. Consider forcing it.");
				return true;
			}
			std::cout << "\n  Choose a spell to target the barrier:\n";
			for (size_t i = 0; i < spells.size(); ++i) {
				std::cout << "    " << (i + 1) << ". " << spells[i].name
					<< " (" << spells[i].GetElementName() << ", Cost: " << spells[i].manaCost << ")\n";
			}
			std::cout << "    0. Cancel\n  > ";
			int sChoice = -1;
			std::cin >> sChoice;
			if (sChoice <= 0 || sChoice > static_cast<int>(spells.size())) {
				Console::PrintSlow("  Cancelled.");
				return true;
			}
			const Spell& sp = spells[sChoice - 1];
			if (player.GetMana() < sp.manaCost) {
				Console::PrintSlow("  Not enough mana to cast that.");
				return true;
			}
			player.UseMana(sp.manaCost);
			int spellDamage = sp.power + player.GetIntelligence() + player.ConsumeAttackBuff(true);

			// If element matches weakness, amplify; Strange + correct element = insta-break
			if (sp.element == weakness) {
				if (mat == WallMaterial::Strange) {
					current.SetHiddenExit(dir, false, 0);
					current.SetExit(dir, true);
					if (ax >= 0 && ax < gridSize_ && ay >= 0 && ay < gridSize_) {
						grid_[ay][ax].SetHiddenExit(OppositeDirection(dir), false, 0);
						grid_[ay][ax].SetExit(OppositeDirection(dir), true);
					}
					Console::PrintSlow("  Your spell resonates with the material and shatters it!");
					switch (dir) { case Direction::North: playerY_--; break; case Direction::South: playerY_++; break; case Direction::East: playerX_++; break; case Direction::West: playerX_--; break; }
					EnterRoom(player);
					return !player.IsAlive() ? false : true;
				}
				else {
					spellDamage = static_cast<int>(spellDamage * 1.75f);
				}
			}

			if (spellDamage >= wallHp) {
				current.SetHiddenExit(dir, false, 0);
				current.SetExit(dir, true);
				if (ax >= 0 && ax < gridSize_ && ay >= 0 && ay < gridSize_) {
					grid_[ay][ax].SetHiddenExit(OppositeDirection(dir), false, 0);
					grid_[ay][ax].SetExit(OppositeDirection(dir), true);
				}
				Console::PrintSlow("  Your spell damages the barrier until it collapses, revealing a passage!");
				switch (dir) { case Direction::North: playerY_--; break; case Direction::South: playerY_++; break; case Direction::East: playerX_++; break; case Direction::West: playerX_--; break; }
				EnterRoom(player);
				return !player.IsAlive() ? false : true;
			}
			else {
				int lost = std::max(1, spellDamage);
				current.hiddenToughness[static_cast<int>(dir)] = std::max(1, wallHp - lost);
				Console::PrintSlow("  The spell scorches the surface but the barrier still stands.");
				return true;
			}
		};

		// Process chosen movement options
		for (const auto& m : moves) {
			if (choice == m.num) {
				// Existing hidden-break path remains but we prefer the new attack flow:
				// If this was a hidden-break attempt, hand off to the new wall attack flow
				if (m.isHidden) {
					if (!HandleWallAttack(m.dir)) return false;
					goto continue_loop;
				}
				// Normal movement along an open exit
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

		// Attack wall chosen
		if (attackWallOpt > 0 && choice == attackWallOpt) {
			// list candidate walls (all dirs without open exit)
			std::vector<Direction> candidates;
			std::cout << "\n  Choose a wall to attack:\n";
			int dirOptBase = 1;
			for (int d = 0; d < 4; ++d) {
				Direction dir = static_cast<Direction>(d);
				if (!current.HasExit(dir)) {
					candidates.push_back(dir);
					std::cout << "    " << dirOptBase << ". " << DirectionName(dir) << "\n";
					dirOptBase++;
				}
			}
			std::cout << "    0. Cancel\n  > ";
			int dirChoice = -1;
			std::cin >> dirChoice;
			if (dirChoice <= 0 || dirChoice > static_cast<int>(candidates.size())) {
				Console::PrintSlow("  Cancelled.");
				continue;
			}
			Direction chosenDir = candidates[dirChoice - 1];
			if (!HandleWallAttack(chosenDir)) return false;
			continue;
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

// ---- Relic offering: the roguelike heart of each run ----
// After clearing a floor, present 3 random relics the player doesn't own.
// Take one, or walk away. Choices are permanent -- choose like it matters.
static void OfferRelicChoice(Player& player, int floorCleared) {
	// Build the pool of unowned relics
	std::vector<RelicId> pool;
	for (const auto& info : AllRelics()) {
		if (!player.HasRelic(info.id)) pool.push_back(info.id);
	}
	if (pool.empty()) return; // Collected everything -- a true dungeon lord

	static RNG relicRng;
	// Shuffle-lite: pick up to 3 distinct offers
	std::vector<RelicId> offers;
	while (offers.size() < 3 && !pool.empty()) {
		int pick = relicRng.NextInt(0, static_cast<int>(pool.size()) - 1);
		offers.push_back(pool[pick]);
		pool.erase(pool.begin() + pick);
	}

	Console::PrintSlow("");
	Console::PrintSlow("  In the rubble of floor " + std::to_string(floorCleared)
		+ ", something glimmers...");
	Console::PrintSlow("  Ancient relics! You may claim ONE. The rest crumble to dust.");
	Console::PrintSlow("");

	for (size_t i = 0; i < offers.size(); ++i) {
		const RelicInfo& info = GetRelicInfo(offers[i]);
		std::cout << "    " << (i + 1) << ". " << info.name << "\n";
		std::cout << "       " << info.description << "\n";
	}
	std::cout << "    " << (offers.size() + 1) << ". Leave them. (No relic)\n";
	std::cout << "    > ";

	int choice = 0;
	std::cin >> choice;
	while (std::cin.fail() || choice < 1 || choice > static_cast<int>(offers.size()) + 1) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "    Enter 1-" << (offers.size() + 1) << ": ";
		std::cin >> choice;
	}

	if (choice <= static_cast<int>(offers.size())) {
		RelicId picked = offers[choice - 1];
		player.GrantRelic(picked);
		const RelicInfo& info = GetRelicInfo(picked);
		Console::PrintSlow("");
		Console::PrintSlow("  ** " + std::string(info.name) + " claimed! **");
		Console::PrintSlow("  " + std::string(info.description));
	}
	else {
		Console::PrintSlow("");
		Console::PrintSlow("  You resist temptation. The relics turn to dust behind you.");
	}
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
	// Removed explicit grid-size line so player stays unsure of layout.
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
		for (int y = 0; y < gridSize_; ++y)
			for (int x = 0; x < gridSize_; ++x)
				if (grid_[y][x].visited) visited++;

		// Show only the raw number of explored rooms (not the total explorable spaces)
		Console::PrintSlow("  Rooms explored: " + std::to_string(visited));
		int bonusXP = visited * 2;
		std::cout << "  Exploration bonus: ";
		player.GainXP(bonusXP);

		Console::PrintSlow("==================================");

		// Roguelike relic choice -- one per floor cleared
		OfferRelicChoice(player, currentLevel_);

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
