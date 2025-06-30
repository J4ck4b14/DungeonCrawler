#include <iostream>
#include "Player.h"
#incldue "Enemy.h"

class Game {
public:
    Game() = default;

    void Run() {
        int choice = 0;

        do {
            // Create new player and enemy each loop
            Player player("Hero", 100, 20);
            Enemy goblin("Goblin", 60, 10);

            // Combat loop
            while (player.GetHP() >= 0 && goblin.GetHP() >= 0) {
                // Initialize enemy and player
                std::cout << "Enemy " << goblin.GetName() << " has " << goblin.GetHP() << " health left!" << std::endl;
                std::cout << player.GetName() << " stands with " << player.GetHP() << std::endl;

                player.TakeTurn();
                goblin.ReceiveDamage(player.GetATK());

                if (goblin.GetHP() <= 0) {
                    std::cout << goblin.GetName() << " has been defeated!" << std::endl;
                    break;
                }

                goblin.TakeTurn();
                player.ReceiveDamage(goblin.GetATK());

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

    Game game;
    game.Run();
    
    return 0;
}