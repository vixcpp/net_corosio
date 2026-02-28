#pragma once

#include <cstdint>

namespace vix::net_corosio
{
  // Semantic version of the module (independent from Vix core).
  inline constexpr std::uint32_t version_major = 0;
  inline constexpr std::uint32_t version_minor = 1;
  inline constexpr std::uint32_t version_patch = 0;

  // Optional: short tag for pre-release builds.
  // Keep empty for stable releases.
  inline constexpr const char *version_prerelease = "";

  // Helper: 0xMMmmpp for fast comparisons (major, minor, patch).
  inline constexpr std::uint32_t version_hex()
  {
    return (version_major << 16) | (version_minor << 8) | (version_patch);
  }

  // "0.1.0" or "0.1.0-alpha"
  inline constexpr const char *version_string()
  {
    // Note: constexpr string building is intentionally avoided here.
    // Keep this stable and edit manually on release.
    return "0.1.0";
  }
} // namespace vix::net_corosio
