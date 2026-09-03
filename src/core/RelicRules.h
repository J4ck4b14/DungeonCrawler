#pragma once

#include "Relic.h"

#include <vector>

namespace RelicRules {

bool IsAvailable(RelicId id, int legacyRank, int floor);
int OfferWeight(RelicId id, int floor);
std::vector<RelicId> BuildEligiblePool(int legacyRank, int floor,
	const std::vector<RelicId>& ownedRelics);

} // namespace RelicRules
