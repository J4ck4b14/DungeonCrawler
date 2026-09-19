#include "dungeon/DungeonTopology.h"
#include "dungeon/DungeonRules.h"
#include "equipment/Equipment.h"
#include "loot/LootGenerator.h"
#include "utils/RNG.h"

#include <array>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <variant>

namespace {
const char* CategoryName(LootCategory category) {
	switch (category) {
	case LootCategory::Common: return "Common";
	case LootCategory::Rare: return "Rare";
	case LootCategory::Epic: return "Epic";
	case LootCategory::Legendary: return "Legendary";
	}
	return "Unknown";
}
}

int main() {
	constexpr int Samples = 50000;
	std::cout << std::fixed << std::setprecision(2);
	std::cout << "ROOM CONTENT DEBUG REPORT (" << Samples << " seeded rolls per row)\n";
	for (int floor : {1, 5, 10, 20, 30, 40, 50}) {
		const RoomContentWeights weights = DungeonRules::CalculateRoomContentWeights(
			floor, 1.0f, 1.0f);
		std::array<int, 5> counts{};
		RNG rng(static_cast<unsigned>(7000 + floor));
		for (int i = 0; i < Samples; ++i) {
			const RoomContent content = DungeonRules::RollRoomContent(weights, rng);
			int index = content == RoomContent::Combat ? 0
				: content == RoomContent::Chest ? 1
				: content == RoomContent::Trap ? 2
				: content == RoomContent::Rest ? 3 : 4;
			++counts[index];
		}
		std::cout << "Floor " << std::setw(2) << floor
			<< ": Combat=" << 100.0 * counts[0] / Samples
			<< "% Chest=" << 100.0 * counts[1] / Samples
			<< "% Trap=" << 100.0 * counts[2] / Samples
			<< "% Rest=" << 100.0 * counts[3] / Samples
			<< "% Empty=" << 100.0 * counts[4] / Samples << "%\n";
	}
	std::cout << '\n';
	std::cout << "LOOT DEBUG REPORT (" << Samples << " seeded drops per row)\n";
	for (int legacy : {0, 9, 17, 30, 40, 50}) {
		const int characterLevel = 1 + legacy / 10;
		RNG rng(9000u + static_cast<unsigned>(legacy));
		std::array<int, 4> categories{};
		int weapons = 0, oneEnchant = 0, twoEnchant = 0;
		double overallRank = 0.0;
		for (int i = 0; i < Samples; ++i) {
			const GeneratedEquipment item = LootGenerator::GenerateEquipment(
				{legacy, characterLevel}, rng);
			const int rank = std::visit([](const auto& equipment) {
				if constexpr (std::is_same_v<std::decay_t<decltype(equipment)>, Weapon>)
					return equipment.GetOverallRank();
				else return equipment.GetItemRank();
			}, item);
			++categories[static_cast<int>(LootGenerator::CategoryForOverallRank(rank))];
			overallRank += rank;
			if (const Weapon* weapon = std::get_if<Weapon>(&item)) {
				++weapons;
				oneEnchant += weapon->GetEnchantments().size() == 1;
				twoEnchant += weapon->GetEnchantments().size() == 2;
			}
		}
		std::cout << "Legacy " << std::setw(2) << legacy << " / run level "
			<< characterLevel << ": ";
		for (int c = 0; c < 4; ++c) {
			std::cout << CategoryName(static_cast<LootCategory>(c)) << '='
				<< 100.0 * categories[c] / Samples << "% ";
		}
		std::cout << "1-enchant=" << 100.0 * oneEnchant / weapons
			<< "% 2-enchant=" << 100.0 * twoEnchant / weapons
			<< "% (weapon drops), avg OR=" << overallRank / Samples << '\n';
	}

	constexpr int Floors = 5000;
	double roomTotal = 0, deadEndTotal = 0, junctionTotal = 0;
	double loopTotal = 0, routeTotal = 0;
	int loopFloors = 0, invalidFloors = 0;
	for (int seed = 1; seed <= Floors; ++seed) {
		RNG rng(static_cast<unsigned>(seed));
		const GeneratedFloorTopology floor = DungeonTopology::Generate(1 + seed % 20, rng);
		invalidFloors += !DungeonTopology::MandatorySpacesConnected(floor);
		loopFloors += floor.metrics.loopCount > 0;
		roomTotal += floor.metrics.roomCount;
		deadEndTotal += floor.metrics.deadEndCount;
		junctionTotal += floor.metrics.junctionCount;
		loopTotal += floor.metrics.loopCount;
		routeTotal += floor.metrics.shortestEntranceToStairs;
	}
	std::cout << "\nDUNGEON DEBUG REPORT (" << Floors << " seeded floors)\n"
		<< "invalid=" << invalidFloors << ", loop floors=" << 100.0 * loopFloors / Floors
		<< "%, avg rooms=" << roomTotal / Floors
		<< ", dead ends=" << deadEndTotal / Floors
		<< ", junctions=" << junctionTotal / Floors
		<< ", loops=" << loopTotal / Floors
		<< ", entrance-stairs=" << routeTotal / Floors << '\n';
	for (unsigned seed : {7u, 42u, 73u}) {
		RNG rng(seed);
		GeneratedFloorTopology floor = DungeonTopology::Generate(8, rng);
		floor.grid[floor.entranceY][floor.entranceX].visited = true;
		std::cout << "\nSeed " << seed << " [rooms=" << floor.metrics.roomCount
			<< ", dead ends=" << floor.metrics.deadEndCount
			<< ", junctions=" << floor.metrics.junctionCount
			<< ", loops=" << floor.metrics.loopCount
			<< ", route=" << floor.metrics.shortestEntranceToStairs << "]\n"
			<< DungeonTopology::RenderKnowledge(floor, floor.entranceX,
				floor.entranceY, true);
	}
	return 0;
}
