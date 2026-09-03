#include "ProfileDisplay.h"

#include "presentation/DisplayUtils.h"
#include "progression/PlayerProfile.h"

#include <iostream>

namespace ProfileDisplay {

void PrintSummary(const PlayerProfile& profile) {
	std::cout << "+-- LEGACY -----------------------------------------------------------+\n";
	std::cout << "| Rank " << profile.GetLegacyRank()
		<< "  XP " << DisplayUtils::MakeMeter(profile.GetXPIntoRank(),
			profile.GetXPForNextRank(), 18)
		<< " " << profile.GetXPIntoRank() << "/" << profile.GetXPForNextRank() << "\n";
	std::cout << "| Runs " << profile.GetRunCount()
		<< "  Highest floor " << profile.GetHighestFloor()
		<< "  Lifetime kills " << profile.GetTotalKills()
		<< "  Relics " << profile.GetUnlockedRelics().size()
		<< "/" << AllRelics().size() << " unlocked\n";
	int nextUnlockRank = 0;
	for (const RelicInfo& info : AllRelics()) {
		if (!profile.IsRelicUnlocked(info.id)
			&& (nextUnlockRank == 0 || info.unlockRank < nextUnlockRank)) {
			nextUnlockRank = info.unlockRank;
		}
	}
	if (nextUnlockRank > 0) {
		std::cout << "| Next relic unlocks at Rank " << nextUnlockRank << ": ";
		bool first = true;
		for (const RelicInfo& info : AllRelics()) {
			if (info.unlockRank != nextUnlockRank || profile.IsRelicUnlocked(info.id)) continue;
			if (!first) std::cout << ", ";
			first = false;
			std::cout << info.name;
		}
		std::cout << "\n";
	}
	else {
		std::cout << "| All current relics unlocked.\n";
	}
	std::cout << "+--------------------------------------------------------------------+\n\n";
}

void PrintRunReward(const PlayerProfile& profile, const LegacyReward& reward) {
	std::cout << "\n+-- LEGACY PROGRESSION -----------------------------------------------+\n";
	std::cout << "| Run XP: +" << reward.xpEarned << "\n";
	std::cout << "| Rank " << profile.GetLegacyRank()
		<< "  " << DisplayUtils::MakeMeter(profile.GetXPIntoRank(),
			profile.GetXPForNextRank(), 18)
		<< " " << profile.GetXPIntoRank() << "/" << profile.GetXPForNextRank() << "\n";
	if (reward.newRank > reward.previousRank) {
		std::cout << "| RANK UP: " << reward.previousRank << " -> " << reward.newRank << "\n";
	}
	for (RelicId id : reward.newlyUnlockedRelics) {
		const RelicInfo& info = GetRelicInfo(id);
		std::cout << "| NEW RELIC UNLOCKED: " << info.name << " ["
			<< RelicRarityName(info.rarity) << "]\n";
	}
	std::cout << "+--------------------------------------------------------------------+\n";
}

void PrintRelicCatalogue(const PlayerProfile& profile) {
	std::cout << "\n+-- LEGACY RELICS ----------------------------------------------------+\n";
	std::cout << "| Rank " << profile.GetLegacyRank() << "  |  "
		<< profile.GetUnlockedRelics().size() << "/" << AllRelics().size()
		<< " unlocked\n";
	std::cout << "+--------------------------------------------------------------------+\n";
	for (const RelicInfo& info : AllRelics()) {
		std::cout << "  [" << (profile.IsRelicUnlocked(info.id) ? "UNLOCKED" : "LOCKED")
			<< "] [" << RelicRarityName(info.rarity) << "] " << info.name;
		if (profile.IsRelicUnlocked(info.id)) {
			std::cout << "  (can appear from floor " << info.minimumFloor << ")\n"
				<< "      " << info.description << "\n";
		}
		else {
			std::cout << "  (Legacy Rank " << info.unlockRank << ")\n";
		}
	}
	std::cout << "+--------------------------------------------------------------------+\n\n";
}

} // namespace ProfileDisplay
