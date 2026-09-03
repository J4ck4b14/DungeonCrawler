#include "AudioMenu.h"

#include "audio/MusicSystem.h"
#include "utils/Console.h"

#include <iostream>
#include <limits>
#include <string>

namespace AudioMenu {

void Open() {
	while (true) {
		std::cout << "\n  MUSIC SETTINGS\n";
		std::cout << "    Volume: " << MusicSystem::GetVolume() << "%"
			<< (MusicSystem::IsMuted() ? " (muted)" : "") << "\n";
		if (!MusicSystem::HasAnyTrack()) {
			std::cout << "    No music files are installed yet. See assets/music/README.md.\n";
		}
		std::cout << "\n";
		std::cout << "    1. Volume up (+10%)\n";
		std::cout << "    2. Volume down (-10%)\n";
		std::cout << "    3. " << (MusicSystem::IsMuted() ? "Unmute" : "Mute") << " music\n";
		std::cout << "    0. Back\n";
		std::cout << "  > ";

		int choice = -1;
		std::cin >> choice;
		while (std::cin.fail() || choice < 0 || choice > 3) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "  Invalid. Enter 0-3: ";
			std::cin >> choice;
		}

		if (choice == 0) return;
		if (choice == 1) MusicSystem::SetVolume(MusicSystem::GetVolume() + 10);
		if (choice == 2) MusicSystem::SetVolume(MusicSystem::GetVolume() - 10);
		if (choice == 3) MusicSystem::ToggleMuted();
		Console::Clear();
	}
}

} // namespace AudioMenu
