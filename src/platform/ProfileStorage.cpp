#include "ProfileStorage.h"

#include "progression/PlayerProfile.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

namespace {

#ifndef __EMSCRIPTEN__
std::string ReadEnvironmentVariable(const char* name) {
#ifdef _WIN32
	char* value = nullptr;
	size_t length = 0;
	if (_dupenv_s(&value, &length, name) != 0 || !value) return {};
	const std::string result(value);
	std::free(value);
	return result;
#else
	const char* value = std::getenv(name);
	return value ? value : "";
#endif
}
#endif

#ifdef __EMSCRIPTEN__
EM_JS(char*, LoadProfileText, (), {
	const text = localStorage.getItem('dungeonCrawlerProfile');
	if (text === null) return 0;
	const size = lengthBytesUTF8(text) + 1;
	const pointer = _malloc(size);
	stringToUTF8(text, pointer, size);
	return pointer;
});

EM_JS(int, SaveProfileText, (const char* text), {
	try {
		localStorage.setItem('dungeonCrawlerProfile', UTF8ToString(text));
		return 1;
	} catch (_) {
		return 0;
	}
});

EM_JS(void, PreserveInvalidProfileText, (const char* text), {
	try {
		localStorage.setItem('dungeonCrawlerProfileInvalid', UTF8ToString(text));
	} catch (_) {}
});
#else
std::filesystem::path ProfilePath() {
#ifdef _WIN32
	if (const std::string localAppData = ReadEnvironmentVariable("LOCALAPPDATA");
		!localAppData.empty()) {
		return std::filesystem::path(localAppData) / "DungeonCrawler" / "profile.dat";
	}
#elif defined(__APPLE__)
	if (const std::string home = ReadEnvironmentVariable("HOME"); !home.empty()) {
		return std::filesystem::path(home) / "Library" / "Application Support"
			/ "DungeonCrawler" / "profile.dat";
	}
#else
	if (const std::string dataHome = ReadEnvironmentVariable("XDG_DATA_HOME");
		!dataHome.empty()) {
		return std::filesystem::path(dataHome) / "dungeoncrawler" / "profile.dat";
	}
	if (const std::string home = ReadEnvironmentVariable("HOME"); !home.empty()) {
		return std::filesystem::path(home) / ".local" / "share"
			/ "dungeoncrawler" / "profile.dat";
	}
#endif
	return std::filesystem::current_path() / "dungeoncrawler-profile.dat";
}
#endif

} // namespace

namespace ProfileStorage {

bool Load(PlayerProfile& profile, std::string* message) {
#ifdef __EMSCRIPTEN__
	char* stored = LoadProfileText();
	if (!stored) {
		if (message) *message = "New Legacy profile created.";
		return true;
	}
	const std::string text(stored);
	std::free(stored);
#else
	const std::filesystem::path path = ProfilePath();
	std::error_code error;
	if (!std::filesystem::exists(path, error)) {
		if (message) *message = "New Legacy profile created.";
		return true;
	}
	std::ifstream input(path, std::ios::binary);
	if (!input) {
		if (message) *message = "The Legacy profile could not be opened.";
		return false;
	}
	std::ostringstream contents;
	contents << input.rdbuf();
	const std::string text = contents.str();
#endif

	std::string parseError;
	if (!PlayerProfile::Deserialize(text, profile, &parseError)) {
#ifdef __EMSCRIPTEN__
		PreserveInvalidProfileText(text.c_str());
		if (message) *message = "The Legacy profile is invalid and was preserved separately: "
			+ parseError;
#else
		std::filesystem::path backup = path.string() + ".invalid";
		for (int suffix = 1; std::filesystem::exists(backup); ++suffix) {
			backup = path.string() + ".invalid." + std::to_string(suffix);
		}
		std::error_code backupError;
		std::filesystem::copy_file(path, backup,
			std::filesystem::copy_options::none, backupError);
		if (message) {
			*message = "The Legacy profile is invalid: " + parseError;
			if (!backupError) *message += " Backup: " + backup.string();
		}
#endif
		return false;
	}
	if (message) *message = "Legacy profile loaded.";
	return true;
}

bool Save(const PlayerProfile& profile, std::string* message) {
	const std::string text = profile.Serialize();
#ifdef __EMSCRIPTEN__
	if (SaveProfileText(text.c_str()) == 0) {
		if (message) *message = "Browser storage rejected the Legacy profile.";
		return false;
	}
#else
	const std::filesystem::path path = ProfilePath();
	const std::filesystem::path temporary = path.string() + ".tmp";
	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);
	if (error) {
		if (message) *message = "Could not create the profile directory.";
		return false;
	}
	{
		std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
		if (!output) {
			if (message) *message = "Could not write the temporary profile file.";
			return false;
		}
		output << text;
		if (!output) {
			if (message) *message = "Writing the Legacy profile failed.";
			return false;
		}
	}
#ifdef _WIN32
	if (!MoveFileExW(temporary.c_str(), path.c_str(),
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
	}
#else
	std::filesystem::rename(temporary, path, error);
#endif
	if (error) {
		if (message) *message = "Could not replace the previous Legacy profile.";
		return false;
	}
#endif
	if (message) *message = "Legacy profile saved.";
	return true;
}

std::string GetDisplayLocation() {
#ifdef __EMSCRIPTEN__
	return "browser local storage";
#else
	return ProfilePath().string();
#endif
}

} // namespace ProfileStorage
