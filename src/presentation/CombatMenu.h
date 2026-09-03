#pragma once

#include "combat/CombatTypes.h"
#include <string>
#include <vector>

class Enemy;
class Player;

namespace CombatMenu {

TurnAction ChooseAction(Player& player);
int ChooseTarget(const std::vector<Enemy>& enemies, const std::string& prompt);

} // namespace CombatMenu
