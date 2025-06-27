#pragma once
#include <string>

class Entity {
public:
	Entity(std::string name, int hp, int atk);
	virtual ~Entity() = default;

	virtual void TakeTurn() = 0;

	const std::string& GetName() const;
	int GetHP() const;
	int GetATK() const;

	void ReceiveDamage(int dmg);

protected:
	std::string name_;
	int hp_;
	int atk_;
};