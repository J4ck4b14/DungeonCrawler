#include "RelicRules.h"

#include <algorithm>

namespace RelicRules {

bool IsAvailable(RelicId id, int legacyRank, int floor) {
	const RelicInfo& info = GetRelicInfo(id);
	return legacyRank >= info.unlockRank && floor >= info.minimumFloor;
}

int OfferWeight(RelicId id, int floor) {
	const RelicRarity rarity = GetRelicInfo(id).rarity;
	switch (rarity) {
	case RelicRarity::Common:
		return std::max(25, 75 - floor * 5);
	case RelicRarity::Uncommon:
		return std::min(55, 25 + floor * 4);
	case RelicRarity::Rare:
		return 10 + std::max(0, floor - 4) * 6;
	}
	return 1;
}

std::vector<RelicId> BuildEligiblePool(int legacyRank, int floor,
	const std::vector<RelicId>& ownedRelics) {
	std::vector<RelicId> pool;
	for (const RelicInfo& info : AllRelics()) {
		if (!IsAvailable(info.id, legacyRank, floor)) continue;
		if (std::find(ownedRelics.begin(), ownedRelics.end(), info.id)
			!= ownedRelics.end()) continue;
		pool.push_back(info.id);
	}
	return pool;
}

} // namespace RelicRules
