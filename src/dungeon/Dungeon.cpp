#include "Dungeon.h"
#include "DungeonRules.h"
#include "DungeonTopology.h"
#include "BreachRules.h"
#include "Perception.h"
#include "entities/EnemyFactory.h"
#include "combat/CombatSystem.h"
#include "combat/EncounterRules.h"
#include "presentation/ExplorationDisplay.h"
#include "presentation/AudioMenu.h"
#include "presentation/ProfileDisplay.h"
#include "audio/MusicSystem.h"
#include "items/Item.h"
#include "utils/RNG.h"
#include "utils/Console.h"
#include "core/DevMode.h"
#include "core/Relic.h"
#include "core/RelicRules.h"
#include "progression/PlayerProfile.h"
#include "combat/Spell.h"
#include "combat/SpellRules.h"
#include "dungeon/RestSite.h"
#include "loot/LootGenerator.h"
#include "presentation/EquipmentMenu.h"
#include "Room.h"
#include <iostream>
#include <limits>
#include <algorithm>

Dungeon::Dungeon(PlayerProfile* profile)
	: currentLevel_(1), gridSize_(0), playerX_(0), playerY_(0), profile_(profile) {}

int Dungeon::GetCurrentLevel() const { return currentLevel_; }
const GameStats& Dungeon::GetStats() const { return gameStats_; }
Bestiary& Dungeon::ActiveBestiary() {
	return profile_ ? profile_->GetBestiary() : fallbackBestiary_;
}
const Bestiary& Dungeon::ActiveBestiary() const {
	return profile_ ? profile_->GetBestiary() : fallbackBestiary_;
}
const Bestiary& Dungeon::GetBestiary() const { return ActiveBestiary(); }

RoomContent Dungeon::GenerateRoomContent(bool isStart, bool isStaircase) {
	if (isStart) return RoomContent::Empty;
	if (isStaircase) return RoomContent::Staircase;

	static RNG rng;

	float enemyScale = 1.0f;
	float trapMul = 1.0f;
	if (DevMode::IsEnabled()) {
		enemyScale = DevMode::GetEnemyScale();
		trapMul = DevMode::GetTrapMultiplier();
	}

	const RoomContentWeights weights = DungeonRules::CalculateRoomContentWeights(
		currentLevel_, enemyScale, trapMul);
	return DungeonRules::RollRoomContent(weights, rng);
}

void Dungeon::GenerateFloor() {
	static RNG topologyRng;
	GeneratedFloorTopology topology = DungeonTopology::Generate(currentLevel_, topologyRng);
	gridSize_ = topology.size;
	playerX_ = topology.entranceX;
	playerY_ = topology.entranceY;
	grid_ = std::move(topology.grid);
	for (int y = 0; y < gridSize_; ++y) {
		for (int x = 0; x < gridSize_; ++x) {
			Room& room = grid_[y][x];
			if (!room.exists) continue;
			const bool isStart = x == playerX_ && y == playerY_;
			const bool isStairs = x == topology.stairsX && y == topology.stairsY;
			room.content = GenerateRoomContent(isStart, isStairs);
		}
	}
	Room& entrance = grid_[playerY_][playerX_];
	entrance.visited = true;
	entrance.contentResolved = true;
	entrance.outcome = RoomOutcome::EmptySearched;
}

void Dungeon::PrintMap() const {
	GeneratedFloorTopology view;
	view.grid = grid_;
	view.size = gridSize_;
	for (int y = 0; y < gridSize_; ++y) for (int x = 0; x < gridSize_; ++x) {
		if (grid_[y][x].content == RoomContent::Staircase) {
			view.stairsX = x;
			view.stairsY = y;
		}
	}
	std::cout << "\n  DUNGEON MAP\n"
		<< "  @ You  o Revealed  . Searched  x Battle  c Chest  r Rest"
		<< "  ! Trap  ^ Disarmed  V Stairs\n\n"
		<< DungeonTopology::RenderKnowledge(view, playerX_, playerY_,
			DevMode::IsEnabled() && DevMode::RevealMapEnabled()) << "\n";
}

bool Dungeon::HandleCombat(Player& player) {
	static RNG rng;
	const int enemyCount = EncounterRules::DetermineEnemyCount(
		currentLevel_, rng.NextInt(1, 100));
	std::vector<Enemy> enemies;
	enemies.reserve(enemyCount);
	for (int i = 0; i < enemyCount; ++i) {
		enemies.push_back(EnemyFactory::CreateEnemy(currentLevel_));
	}
	MusicSystem::Play(MusicSystem::Scene::Combat);
	const bool won = CombatSystem::ResolveCombat(
		player, enemies, seenEnemyTypes_, gameStats_, ActiveBestiary());
	if (player.IsAlive()) MusicSystem::Play(MusicSystem::Scene::Exploration);
	return won;
}

void Dungeon::HandleChest(Player& player) {
	static RNG rng;
	int roll = rng.NextInt(1, 100);

	gameStats_.chestsOpened++;
	Console::PrintSlow("  You find a chest and pry it open...");

	if (roll <= 25) {
		Item potion = (currentLevel_ >= 5) ? MakeLargeHealthPotion() : MakeHealthPotion();
		player.GetInventory().AddItem(potion);
		Console::PrintSlow("  Found: " + potion.name + "!");
	}
	else if (roll <= 45) {
		Item potion = (currentLevel_ >= 5) ? MakeLargeManaPotion() : MakeManaPotion();
		player.GetInventory().AddItem(potion);
		Console::PrintSlow("  Found: " + potion.name + "!");
	}
	else if (roll <= 55) {
		Item hp = MakeHealthPotion();
		Item mp = MakeManaPotion();
		player.GetInventory().AddItem(hp);
		player.GetInventory().AddItem(mp);
		Console::PrintSlow("  Jackpot! Found: " + hp.name + " and " + mp.name + "!");
	}
	else if (roll <= 90) {
		const LootContext context{
			profile_ ? profile_->GetLegacyRank() : 1,
			player.GetLevel()
		};
		Console::PrintSlow("  Inside lies a piece of equipment.");
		if (EquipmentMenu::Offer(player,
			LootGenerator::GenerateEquipment(context, rng))) {
			Console::PrintSlow("  Equipped.");
		} else {
			Console::PrintSlow("  You leave it behind.");
		}
	}
	else {
		Console::PrintSlow("  The chest is empty. How disappointing.");
	}
}

void Dungeon::HandleRest(Player& player) {
	Console::PrintSlow("  You find a quiet alcove. What do you do?");
	RestSite site;
	while (!site.IsConsumed()) {
		std::cout << "\n    1. Rest (restore HP and Mana)\n"
			<< "    2. Train (+1 STR, SPD, or INT)\n"
			<< "    3. Sharpen equipped weapon (+1 weapon rank)\n"
			<< "    4. Improve equipped apparel (+1 item rank)\n  > ";
		int choice = 0;
		std::cin >> choice;
		if (std::cin.fail() || choice < 1 || choice > 4) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			Console::PrintSlow("  Invalid. Enter 1-4.");
			continue;
		}

		if (choice == 1) {
			site.Use(player, RestAction::Rest, currentLevel_);
			Console::PrintSlow("  You rest and recover " + std::to_string(5 + currentLevel_)
				+ " HP and " + std::to_string(3 + currentLevel_ / 2) + " Mana.");
		}
		else if (choice == 2) {
			std::cout << "  Choose a stat:\n    1. Strength\n    2. Speed\n"
				<< "    3. Intelligence\n  > ";
			int stat = 0;
			std::cin >> stat;
			if (std::cin.fail() || stat < 1 || stat > 3) {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				Console::PrintSlow("  No training is consumed. Choose again.");
				continue;
			}
			const RestAction action = stat == 1 ? RestAction::TrainStrength
				: stat == 2 ? RestAction::TrainSpeed : RestAction::TrainIntelligence;
			site.Use(player, action, currentLevel_);
		}
		else if (choice == 3) {
			if (!site.Use(player, RestAction::SharpenWeapon, currentLevel_)) {
				Console::PrintSlow("  You have no weapon to sharpen. Choose another action.");
				continue;
			}
			Console::PrintSlow("  The weapon's edge improves permanently for this run.");
			EquipmentMenu::PrintLoadout(player);
		}
		else {
			std::vector<ApparelSlot> equippedSlots;
			for (ApparelSlot slot : {ApparelSlot::Head, ApparelSlot::Hands,
				ApparelSlot::Torso, ApparelSlot::Legs, ApparelSlot::Feet}) {
				if (player.GetEquipment().GetApparel(slot)) equippedSlots.push_back(slot);
			}
			if (equippedSlots.empty()) {
				Console::PrintSlow("  You have no apparel to improve. Choose another action.");
				continue;
			}
			std::cout << "  Choose apparel:\n";
			for (std::size_t i = 0; i < equippedSlots.size(); ++i) {
				std::cout << "    " << i + 1 << ". "
					<< player.GetEquipment().GetApparel(equippedSlots[i])->GetDisplayName() << "\n";
			}
			std::cout << "  > ";
			int apparelChoice = 0;
			std::cin >> apparelChoice;
			if (std::cin.fail() || apparelChoice < 1
				|| apparelChoice > static_cast<int>(equippedSlots.size())) {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				Console::PrintSlow("  No improvement is consumed. Choose again.");
				continue;
			}
			const ApparelSlot slot = equippedSlots[static_cast<std::size_t>(apparelChoice - 1)];
			site.Use(player, RestAction::ImproveApparel, currentLevel_, slot);
			Console::PrintSlow("  Improved: "
				+ player.GetEquipment().GetApparel(slot)->GetDisplayName() + ".");
		}
	}
}

RoomOutcome Dungeon::HandleTrap(Player& player, Room& room) {
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
		return RoomOutcome::TrapDisarmed;
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
		return RoomOutcome::TrapTriggered;
	}
}

void Dungeon::HandleRoomContent(Player& player, Room& room) {
	if (room.contentResolved) {
		std::cout << "  This room has already been cleared.\n";
		return;
	}

	RoomOutcome outcome = RoomOutcome::Unresolved;
	switch (room.content) {
	case RoomContent::Combat:
		Console::PrintSlow("  Something stirs in the shadows...");
		Console::WaitForEnter();
		if (HandleCombat(player)) outcome = RoomOutcome::EnemyDefeated;
		break;
	case RoomContent::Chest:
		HandleChest(player);
		outcome = RoomOutcome::ChestOpened;
		break;
	case RoomContent::Rest:
		HandleRest(player);
		outcome = RoomOutcome::Rested;
		break;
	case RoomContent::Trap:
		outcome = HandleTrap(player, room);
		break;
	case RoomContent::Staircase:
		Console::PrintSlow("  You find a staircase spiraling downward into darkness.");
		return;
	case RoomContent::Empty:
		Console::PrintSlow("  The room is empty. Dust motes drift in the stale air.");
		outcome = RoomOutcome::EmptySearched;
		break;
	}

	room.contentResolved = true;
	room.outcome = outcome;
	if (player.IsAlive()) {
		gameStats_.roomsExplored++;
	}
}

void Dungeon::EnterRoom(Player& player) {
	Room& room = grid_[playerY_][playerX_];
	room.visited = true;

	Console::WaitForEnter();
	Console::Clear();

	HandleRoomContent(player, room);
}

Dungeon::MovementResult Dungeon::PromptMovement(Player& player) {
	while (player.IsAlive()) {
		Room& current = grid_[playerY_][playerX_];

		int visitedRooms = 0;
		int existingRooms = 0;
		for (const auto& row : grid_) {
			for (const Room& room : row) {
				if (room.exists) ++existingRooms;
				if (room.visited) ++visitedRooms;
			}
		}
		ExplorationDisplay::PrintStatusPanel(currentLevel_, playerX_, playerY_,
			visitedRooms, existingRooms, current, player);
		PrintMap();

		std::cout << "\n  TRAVEL\n";
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
			else if (current.HasHiddenExit(dir) && current.IsWallSuspected(dir)) {
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

		std::cout << "\n  ACTIONS\n";
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
		perceiveOpt = optNum;
		std::cout << "    " << optNum << ". "
			<< (current.perceptionUsed
				? "Review surroundings (recall survey)"
				: "Check environment (survey surroundings)")
			<< "\n";
		optNum++;

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
		std::cout << "\n  JOURNAL\n";
		int bestiaryOpt = optNum;
		std::cout << "    " << optNum << ". Bestiary (" << ActiveBestiary().GetEntryCount() << " entries)\n";
		optNum++;
		int legacyOpt = 0;
		if (profile_) {
			legacyOpt = optNum;
			std::cout << "    " << optNum << ". Legacy relics (Rank "
				<< profile_->GetLegacyRank() << ")\n";
			optNum++;
		}
		int audioOpt = optNum;
		std::cout << "    " << optNum << ". Music settings ("
			<< MusicSystem::GetVolume() << "%"
			<< (MusicSystem::IsMuted() ? ", muted" : "") << ")\n";
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

		// Wall attack handler: does physical or magical attempts, mirrors earlier logic
		enum class WallAttackResult { ContinueExploring, PlayerDied };
		auto HandleWallAttack = [&](Direction dir) -> WallAttackResult {
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
					return WallAttackResult::ContinueExploring;
				}
			}
			if (!current.IsWallBreakable(dir)) {
				Console::PrintSlow("\n  You test the " + std::string(DirectionName(dir))
					+ " wall, but it is useless solid masonry. No route appears.");
				return WallAttackResult::ContinueExploring;
			}
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
			// Cancelling a wall attack is a normal menu action
			if (sub == 0) return WallAttackResult::ContinueExploring;

			int ax = playerX_, ay = playerY_;
			switch (dir) { case Direction::North: ay--; break; case Direction::South: ay++; break; case Direction::East: ax++; break; case Direction::West: ax--; break; }
			auto SetWallToughness = [&](int toughness) {
				current.SetHiddenToughness(dir, toughness);
				if (ax >= 0 && ax < gridSize_ && ay >= 0 && ay < gridSize_) {
					grid_[ay][ax].SetHiddenToughness(OppositeDirection(dir), toughness);
				}
			};

			if (sub == 1) {
				int rawRoll = rng.NextInt(1, 20);
				const char* weaponArchetype = nullptr;
				if (player.GetEquipment().GetWeapon()) {
					weaponArchetype = EquipmentRules::WeaponArchetypeName(
						player.GetEquipment().GetWeapon()->GetArchetype()).data();
				}
				const BreachAttempt attempt = BreachRules::ResolvePhysical(
					current.GetHiddenWall(dir), player.GetStrength(), rawRoll, weaponArchetype);
				Console::PrintSlow("\n  You strike the barrier with all your might...");
				if (rawRoll == 20) Console::PrintSlow("  A perfect hit! You land a heavy blow!");
				if (rawRoll == 1) Console::PrintSlow("  Your blow slips and hurts you!");

				if (attempt.outcome == BreachOutcome::Opened) {
					// break both sides
					current.ClearHiddenWall(dir);
					current.SetExit(dir, true);
					if (ax >= 0 && ax < gridSize_ && ay >= 0 && ay < gridSize_) {
						grid_[ay][ax].ClearHiddenWall(OppositeDirection(dir));
						grid_[ay][ax].SetExit(OppositeDirection(dir), true);
						grid_[ay][ax].mapRevealed = true;
					}
					Console::PrintSlow("  Your strike breaks the barrier! A passage opens.");
					// move player into adjacent room
					switch (dir) { case Direction::North: playerY_--; break; case Direction::South: playerY_++; break; case Direction::East: playerX_++; break; case Direction::West: playerX_--; break; }
					EnterRoom(player);
					return player.IsAlive()
						? WallAttackResult::ContinueExploring
						: WallAttackResult::PlayerDied;
				}
				else {
					SetWallToughness(std::max(1, attempt.remainingToughness));
					Console::PrintSlow("  The blow chips at the barrier, but it holds.");
					if (rawRoll == 1 || rng.Chance(0.18f)) {
						int dmg = rng.NextInt(1, 3) + (currentLevel_ / 2);
						player.ReceiveDamage(dmg);
						gameStats_.totalDamageTaken += dmg;
						Console::PrintSlow("  You strain yourself and take " + std::to_string(dmg) + " damage!");
						if (!player.IsAlive()) {
							Console::PrintSlow("  The effort proved fatal...");
							return WallAttackResult::PlayerDied;
						}
						else {
							player.PrintStatus();
						}
					}
					return WallAttackResult::ContinueExploring;
				}
			}

			// Cast spell at wall
			const auto& spells = player.GetKnownSpells();
			std::vector<int> damagingSpellIndices;
			for (size_t i = 0; i < spells.size(); ++i) {
				if (spells[i].effect == SpellEffect::Damage) {
					damagingSpellIndices.push_back(static_cast<int>(i));
				}
			}
			if (damagingSpellIndices.empty()) {
				Console::PrintSlow("  You don't know any damaging spells to try. Consider forcing it.");
				return WallAttackResult::ContinueExploring;
			}
			std::cout << "\n  Choose a spell to target the barrier:\n";
			for (size_t i = 0; i < damagingSpellIndices.size(); ++i) {
				const Spell& candidate = spells[damagingSpellIndices[i]];
				int manaCost = player.GetEffectiveManaCost(candidate);
				std::cout << "    " << (i + 1) << ". " << candidate.name
					<< " (" << candidate.GetElementName() << ", Cost: " << manaCost << ")\n";
			}
			std::cout << "    0. Cancel\n  > ";
			int sChoice = -1;
			std::cin >> sChoice;
			if (sChoice <= 0 || sChoice > static_cast<int>(damagingSpellIndices.size())) {
				Console::PrintSlow("  Cancelled.");
				return WallAttackResult::ContinueExploring;
			}
			const Spell& sp = spells[damagingSpellIndices[sChoice - 1]];
			int manaCost = player.GetEffectiveManaCost(sp);
			if (player.GetMana() < manaCost) {
				Console::PrintSlow("  Not enough mana to cast that.");
				return WallAttackResult::ContinueExploring;
			}
			player.UseMana(manaCost);
			int spellDamage = SpellRules::CalculateDamage(sp, player.GetStrength(),
				player.GetSpeed(), player.GetIntelligence(), player.GetLevel());
			spellDamage += player.GetSpellPowerBonus();

			const BreachAttempt spellAttempt = BreachRules::ResolveSpell(
				current.GetHiddenWall(dir), spellDamage, sp.element);
			if (spellAttempt.outcome == BreachOutcome::Opened) {
				current.ClearHiddenWall(dir);
				current.SetExit(dir, true);
				if (ax >= 0 && ax < gridSize_ && ay >= 0 && ay < gridSize_) {
					grid_[ay][ax].ClearHiddenWall(OppositeDirection(dir));
					grid_[ay][ax].SetExit(OppositeDirection(dir), true);
					grid_[ay][ax].mapRevealed = true;
				}
				Console::PrintSlow("  Your spell damages the barrier until it collapses, revealing a passage!");
				switch (dir) { case Direction::North: playerY_--; break; case Direction::South: playerY_++; break; case Direction::East: playerX_++; break; case Direction::West: playerX_--; break; }
				EnterRoom(player);
				return player.IsAlive()
					? WallAttackResult::ContinueExploring
					: WallAttackResult::PlayerDied;
			}
			else {
				SetWallToughness(std::max(1, spellAttempt.remainingToughness));
				Console::PrintSlow("  The spell scorches the surface but the barrier still stands.");
				return WallAttackResult::ContinueExploring;
			}
		};

		// Process chosen movement options
		for (const auto& m : moves) {
			if (choice == m.num) {
				// Existing hidden-break path remains but we prefer the new attack flow:
				// If this was a hidden-break attempt, hand off to the new wall attack flow
				if (m.isHidden) {
					if (HandleWallAttack(m.dir) == WallAttackResult::PlayerDied) {
						return MovementResult::PlayerDied;
					}
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
				if (!player.IsAlive()) return MovementResult::PlayerDied;
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
			if (HandleWallAttack(chosenDir) == WallAttackResult::PlayerDied) {
				return MovementResult::PlayerDied;
			}
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
			ActiveBestiary().Print();
			continue;
		}

		if (legacyOpt > 0 && choice == legacyOpt) {
			ProfileDisplay::PrintRelicCatalogue(*profile_);
			continue;
		}

		if (choice == audioOpt) {
			AudioMenu::Open();
			continue;
		}

		if (descOpt > 0 && choice == descOpt) {
			current.contentResolved = true;
			return MovementResult::ReachedStairs;
		}

	continue_loop:;
	}

	return MovementResult::PlayerDied;
}

// ---- Relic offering: the roguelike heart of each run ----
// After clearing a floor, present up to 3 unlocked, depth-appropriate relics.
// Take one, or walk away. Choices are permanent -- choose like it matters.
static void OfferRelicChoice(Player& player, int floorCleared,
	const PlayerProfile* profile) {
	const int legacyRank = profile ? profile->GetLegacyRank()
		: PlayerProfile::MaximumLegacyRank;
	std::vector<RelicId> pool = RelicRules::BuildEligiblePool(
		legacyRank, floorCleared, player.GetRelics());
	if (pool.empty()) return; // Collected everything -- a true dungeon lord

	static RNG relicRng;
	// Weighted sampling without replacement. Common relics dominate early
	// floors; uncommon and rare weights increase as the run deepens.
	std::vector<RelicId> offers;
	while (offers.size() < 3 && !pool.empty()) {
		int totalWeight = 0;
		for (RelicId id : pool) totalWeight += RelicRules::OfferWeight(id, floorCleared);
		int roll = relicRng.NextInt(1, totalWeight);
		size_t pick = 0;
		for (; pick + 1 < pool.size(); ++pick) {
			roll -= RelicRules::OfferWeight(pool[pick], floorCleared);
			if (roll <= 0) break;
		}
		offers.push_back(pool[pick]);
		pool.erase(pool.begin() + static_cast<std::ptrdiff_t>(pick));
	}

	Console::PrintSlow("");
	Console::PrintSlow("  In the rubble of floor " + std::to_string(floorCleared)
		+ ", something glimmers...");
	Console::PrintSlow("  Ancient relics! You may claim ONE. The rest crumble to dust.");
	Console::PrintSlow("");

	for (size_t i = 0; i < offers.size(); ++i) {
		const RelicInfo& info = GetRelicInfo(offers[i]);
		std::cout << "    " << (i + 1) << ". [" << RelicRarityName(info.rarity)
			<< "] " << info.name << "\n";
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

FloorResult Dungeon::RunFloor(Player& player) {
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

	MovementResult movementResult = PromptMovement(player);

	if (movementResult == MovementResult::PlayerDied) return FloorResult::PlayerDied;

	if (movementResult == MovementResult::ReachedStairs) {
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
		Console::PrintSlow("  Exploration itself grants no XP; survival is reward enough.");

		Console::PrintSlow("==================================");

		// Roguelike relic choice -- one per floor cleared
		OfferRelicChoice(player, currentLevel_, profile_);

		int restHp = 5 + currentLevel_;
		int restMana = 3 + currentLevel_ / 2;
		player.Heal(restHp);
		player.RestoreMana(restMana);
		Console::PrintSlow("  Between floors you rest, recovering " + std::to_string(restHp)
			+ " HP and " + std::to_string(restMana) + " Mana.");
		player.PrintStatus();

		currentLevel_++;
		return FloorResult::Cleared;
	}

	return FloorResult::PlayerDied;
}
