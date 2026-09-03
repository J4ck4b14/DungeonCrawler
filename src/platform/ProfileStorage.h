#pragma once

#include <string>

class PlayerProfile;

namespace ProfileStorage {

// Missing profiles are not errors: Load leaves the supplied default profile
// intact and returns true. A malformed or unsupported file returns false.
bool Load(PlayerProfile& profile, std::string* message = nullptr);
bool Save(const PlayerProfile& profile, std::string* message = nullptr);
std::string GetDisplayLocation();

} // namespace ProfileStorage
