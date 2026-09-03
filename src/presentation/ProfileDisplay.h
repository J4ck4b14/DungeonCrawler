#pragma once

class PlayerProfile;
struct LegacyReward;

namespace ProfileDisplay {

void PrintSummary(const PlayerProfile& profile);
void PrintRunReward(const PlayerProfile& profile, const LegacyReward& reward);
void PrintRelicCatalogue(const PlayerProfile& profile);

} // namespace ProfileDisplay
