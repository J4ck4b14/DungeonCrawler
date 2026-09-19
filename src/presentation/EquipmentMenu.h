#pragma once

#include "loot/LootGenerator.h"

class Player;

namespace EquipmentMenu {

// Offers a found item for its matching slot. Returns true when equipped.
bool Offer(Player& player, GeneratedEquipment equipment);
void PrintLoadout(const Player& player);

} // namespace EquipmentMenu
