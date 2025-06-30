#pragma once
#include "Entity.h"

class Player : public Entity {
public:
	Player(const std::string& name, int hp, int atk);

	void TakeTurn() override;
};