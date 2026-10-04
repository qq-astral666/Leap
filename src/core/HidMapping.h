#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace leap {

// Caps Lock can't be swallowed by an event tap (the LED and the lock state
// toggle below it), so Leap asks the HID driver to report it as F18 instead:
//   hidutil property --set '{"UserKeyMapping":[{"HIDKeyboardModifierMappingSrc":..,"...Dst":..}]}'
// The mapping lives until reboot and replaces the whole list, so other
// mappings the user has (Karabiner-less remaps) are read first and kept.

struct HidMapping {
    std::uint64_t src = 0;
    std::uint64_t dst = 0;
    bool operator==(const HidMapping&) const = default;
};

constexpr std::uint64_t kHidCapsLock = 0x700000039;
constexpr std::uint64_t kHidF18 = 0x70000006D;

// Parses `hidutil property --get UserKeyMapping` (an NSArray description).
std::vector<HidMapping> parseHidutilMappings(std::string_view output);

// The list with Caps Lock -> F18 added (enable) or removed (disable).
std::vector<HidMapping> withCapsLockRemap(std::vector<HidMapping> list, bool enable);

// JSON for `hidutil property --set`.
std::string hidutilSetJson(const std::vector<HidMapping>& list);

} // namespace leap
