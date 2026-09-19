#pragma once

#include "equipment/Equipment.h"

#include <optional>

class Player;

enum class RestAction {
	Rest,
	TrainStrength,
	TrainSpeed,
	TrainIntelligence,
	SharpenWeapon,
	ImproveApparel
};

class RestSite {
public:
	bool IsConsumed() const;
	bool Use(Player& player, RestAction action, int floor,
		std::optional<ApparelSlot> apparelSlot = std::nullopt);
private:
	bool consumed_ = false;
};
