#include "MusicSystem.h"

#include <algorithm>
#include <filesystem>
#include <string>

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace MusicSystem {
namespace {

constexpr int kDefaultVolume = 70;
int volume = kDefaultVolume;
bool muted = false;
Scene currentScene = Scene::None;
bool playing = false;

int EffectiveVolume() {
	return muted ? 0 : volume;
}

#ifndef __EMSCRIPTEN__
std::filesystem::path FindTrack(Scene scene) {
	const std::filesystem::path relative(GetTrackPath(scene));
	if (relative.empty()) return {};

	std::error_code error;
	std::filesystem::path base = std::filesystem::current_path(error);
	if (error) return {};

	for (int depth = 0; depth < 6; ++depth) {
		const std::filesystem::path candidate = base / relative;
		if (std::filesystem::is_regular_file(candidate, error) && !error) {
			return std::filesystem::absolute(candidate, error);
		}
		error.clear();
		if (!base.has_parent_path() || base.parent_path() == base) break;
		base = base.parent_path();
	}
	return {};
}
#endif

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
constexpr wchar_t kMusicAlias[] = L"dungeon_music";

void ApplyNativeVolume() {
	const std::wstring command = std::wstring(L"setaudio ") + kMusicAlias
		+ L" volume to " + std::to_wstring(EffectiveVolume() * 10);
	mciSendStringW(command.c_str(), nullptr, 0, nullptr);
}

void StopBackend() {
	const std::wstring command = std::wstring(L"close ") + kMusicAlias;
	mciSendStringW(command.c_str(), nullptr, 0, nullptr);
}

bool PlayBackend(const std::filesystem::path& path) {
	StopBackend();
	const std::wstring type = path.extension() == L".wav" ? L"waveaudio" : L"mpegvideo";
	const std::wstring openCommand = L"open \"" + path.wstring() + L"\" type "
		+ type + L" alias " + kMusicAlias;
	if (mciSendStringW(openCommand.c_str(), nullptr, 0, nullptr) != 0) return false;

	ApplyNativeVolume();
	const std::wstring playCommand = std::wstring(L"play ") + kMusicAlias + L" repeat";
	if (mciSendStringW(playCommand.c_str(), nullptr, 0, nullptr) != 0) {
		StopBackend();
		return false;
	}
	return true;
}
#elif defined(__EMSCRIPTEN__)
EM_JS(int, PlayWebMusic, (const char* path, int volumePercent), {
	try {
		if (Module.dungeonMusic) {
			Module.dungeonMusic.pause();
			if (Module.dungeonMusicUrl) URL.revokeObjectURL(Module.dungeonMusicUrl);
		}
		const bytes = FS.readFile(UTF8ToString(path));
		const url = URL.createObjectURL(new Blob([bytes], {type: 'audio/mpeg'}));
		const audio = new Audio(url);
		audio.loop = true;
		audio.volume = Math.max(0, Math.min(1, volumePercent / 100));
		Module.dungeonMusic = audio;
		Module.dungeonMusicUrl = url;

		const begin = () => audio.play().catch(() => {});
		audio.play().catch(() => {
			document.addEventListener('pointerdown', begin, {once: true});
			document.addEventListener('keydown', begin, {once: true});
		});
		return 1;
	} catch (_) {
		return 0;
	}
});

EM_JS(void, StopWebMusic, (), {
	if (Module.dungeonMusic) {
		Module.dungeonMusic.pause();
		Module.dungeonMusic = null;
	}
	if (Module.dungeonMusicUrl) {
		URL.revokeObjectURL(Module.dungeonMusicUrl);
		Module.dungeonMusicUrl = null;
	}
});

EM_JS(void, SetWebMusicVolume, (int volumePercent), {
	if (Module.dungeonMusic) {
		Module.dungeonMusic.volume = Math.max(0, Math.min(1, volumePercent / 100));
	}
});

void ApplyNativeVolume() {
	SetWebMusicVolume(EffectiveVolume());
}

void StopBackend() {
	StopWebMusic();
}

bool PlayBackend(const std::filesystem::path&) {
	const std::string path = "/" + std::string(GetTrackPath(currentScene));
	return PlayWebMusic(path.c_str(), EffectiveVolume()) != 0;
}
#else
void ApplyNativeVolume() {}
void StopBackend() {}
bool PlayBackend(const std::filesystem::path&) { return false; }
#endif

} // namespace

std::string_view GetTrackPath(Scene scene) {
	switch (scene) {
	case Scene::Title: return "assets/music/title.mp3";
	case Scene::Exploration: return "assets/music/exploration.mp3";
	case Scene::Combat: return "assets/music/combat.mp3";
	case Scene::None: return {};
	}
	return {};
}

bool HasTrack(Scene scene) {
#ifdef __EMSCRIPTEN__
	// Emscripten's packaged virtual filesystem is checked when PlayWebMusic runs.
	return scene != Scene::None;
#else
	return !FindTrack(scene).empty();
#endif
}

bool HasAnyTrack() {
	return HasTrack(Scene::Title)
		|| HasTrack(Scene::Exploration)
		|| HasTrack(Scene::Combat);
}

bool Play(Scene scene) {
	if (scene == Scene::None) {
		Stop();
		return true;
	}
	if (playing && currentScene == scene) return true;

#ifdef __EMSCRIPTEN__
	const std::filesystem::path packagedPath;
#else
	const std::filesystem::path packagedPath = FindTrack(scene);
	if (packagedPath.empty()) {
		return false;
	}
#endif

	StopBackend();
	playing = false;
	currentScene = scene;

	playing = PlayBackend(packagedPath);
	if (!playing) currentScene = Scene::None;
	return playing;
}

void Stop() {
	StopBackend();
	playing = false;
	currentScene = Scene::None;
}

void SetVolume(int percent) {
	volume = std::clamp(percent, 0, 100);
	ApplyNativeVolume();
}

int GetVolume() {
	return volume;
}

void SetMuted(bool shouldMute) {
	muted = shouldMute;
	ApplyNativeVolume();
}

void ToggleMuted() {
	SetMuted(!muted);
}

bool IsMuted() {
	return muted;
}

} // namespace MusicSystem
