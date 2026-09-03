#pragma once

#include <string_view>

namespace MusicSystem {

enum class Scene {
	None,
	Title,
	Exploration,
	Combat
};

// Playback is intentionally best-effort: missing music files leave the game
// fully playable and can be added later without recompiling game rules.
bool Play(Scene scene);
void Stop();

void SetVolume(int percent);
int GetVolume();
void SetMuted(bool muted);
void ToggleMuted();
bool IsMuted();

bool HasTrack(Scene scene);
bool HasAnyTrack();
std::string_view GetTrackPath(Scene scene);

} // namespace MusicSystem
