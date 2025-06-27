#include "Entity.h"

Entity::Entity(std::string name, int hp, int atk):name_(std::move(name)), hp_(hp), atk_(atk){}

const std::string& Entity::GetName() const { return name_; }
int Entity::GetHP() const { return hp_; }
int Entity::GetATK() const { return atk_; }

void Entity::ReceiveDamage(int dmg) {
	hp_ -= dmg;
	if (hp_ < 0) hp_ = 0;
}