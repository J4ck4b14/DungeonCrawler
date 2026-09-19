#include "RestSite.h"

#include "entities/Player.h"

#include <algorithm>

bool RestSite::IsConsumed() const { return consumed_; }

bool RestSite::Use(Player& player, RestAction action, int floor,
	std::optional<ApparelSlot> apparelSlot) {
	if (consumed_) return false;
	bool applied = false;
	switch (action) {
	case RestAction::Rest:
		player.Heal(5 + std::max(1, floor));
		player.RestoreMana(3 + std::max(1, floor) / 2);
		applied = true;
		break;
	case RestAction::TrainStrength: applied = player.TrainStat(1); break;
	case RestAction::TrainSpeed: applied = player.TrainStat(2); break;
	case RestAction::TrainIntelligence: applied = player.TrainStat(3); break;
	case RestAction::SharpenWeapon: applied = player.SharpenWeapon(); break;
	case RestAction::ImproveApparel:
		applied = apparelSlot.has_value() && player.ImproveApparel(*apparelSlot);
		break;
	}
	if (applied) consumed_ = true;
	return applied;
}
