#pragma once
#include "Entity.h"

class Enemy : public Entity {
public:
	Enemy(const std::string& name, const Stats& stats, const std::vector<Spell>& spells = {});

	TurnAction DecideTurn() override;

	void PrintStatus() const;
};