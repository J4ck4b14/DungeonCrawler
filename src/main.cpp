#include <iostream>
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "entities/EnemyFactory.h"
#include "utils/RNG.h"

class Game {
public:
    Game() = default;

    void Run() {
        int choice = 0;

        for (int i = 0; i < 10; i++) {
            Enemy testEnemy = EnemyFactory::CreateRandomEnemy();
            std::cout << "Generated Enemy: " << testEnemy.GetName()
                << " | HP: " << testEnemy.GetHP()
                << " | ATK: " << testEnemy.GetATK() << std::endl;
        }
        

        RNG rng;
        int successes = 0;
        for (int i = 0; i < 100; ++i) {
            if (rng.Chance(0.25f)) ++successes;
        }
        std::cout << "Success rate: " << successes << "/100" << std::endl;


        do {
            // Create new player and enemy each loop
            Player player("Hero", 100, 20);
			Enemy enemy = EnemyFactory::CreateRandomEnemy();

            // Combat loop
            while (player.GetHP() >= 0 && enemy.GetHP() >= 0) {
                // Initialize enemy and player
                std::cout << "Enemy " << enemy.GetName() << " has " << enemy.GetHP() << " health left!" << std::endl;
                std::cout << player.GetName() << " stands with " << player.GetHP() << std::endl;

                player.TakeTurn();
                enemy.ReceiveDamage(player.GetATK());

                if (enemy.GetHP() <= 0) {
                    std::cout << enemy.GetName() << " has been defeated!" << std::endl;
                    break;
                }

                enemy.TakeTurn();
                player.ReceiveDamage(enemy.GetATK());

                if (player.GetHP() <= 0) {
                    std::cout << player.GetName() << " has died in fierce combat.\n"
                        << "Their legend will live on.\n" << std::endl;
                    break;
                }
            }

            std::cout << "Try again?\n"
                << "1. Yes\n"
                << "2. No" << std::endl;
            std::cin >> choice;

            while (choice != 1 && choice != 2) {
                std::cout << "Invalid input. Choose 1 (try again) or 2 (give up)." << std::endl;
                std::cin >> choice;
            }
        } while (choice == 1);

        std::cout << "\nThanks for playing DungeonCrawler!\n";
    };
};

int main() {

    unsigned int seed = 42; // The meaning of life, death and everything else as a seed
	RNG rng(seed); // Initialize RNG with a seed

	Game game; // Pass the RNG to the game constructor
    game.Run();
    
    return 0;
}