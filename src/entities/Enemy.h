#pragma once
#include "Entity.h"

class Enemy : public Entity {
public:
	Enemy(const std::string& name, int hp, int atk);

	void TakeTurn() override;
};