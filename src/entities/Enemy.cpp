#include "Enemy.h"
#include <iostream>

// Constructor simply forwards parameters to base class
Enemy::Enemy(const std::string& name, int hp, int atk)
	:Entity(name, hp,atk){}

//Simulates a simple attack (pass in the target later)
void Enemy::TakeTurn(){
	std::cout << name_ << " snarls and prepares to attack!" << std::endl;
	//Placeholder logic for now
	std::cout << name_ << " attacks with " << atk_ << " damage!" << std::endl;
}