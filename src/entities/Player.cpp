#include "Player.h"
#include <iostream>

Player::Player(const std::string& name, int hp, int atk)
	:Entity(name, hp, atk){}

void Player::TakeTurn() {

	int choice = 0;

	std::cout << name_ << " is deciding what to do.\n"
		<< "\t1.Attack\n"
		<< "\t2.Defend" << std::endl;
	std::cin >> choice;

	while (choice != 1 && choice != 2) {
		std::cout << "Invalid choice. Please enter 1 or 2." << std::endl;
		std::cin >> choice;
	}

	if (choice == 1) {
		std::cout << name_ << " is attacking with " << atk_ << " power!" << std::endl;
	}
	else std::cout << name_ << " prepares for the attack!";
}