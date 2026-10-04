#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace leap {

// A physical key: the macOS virtual keycode (kVK_*). It names the key
// position, not the character, so bindings work the same in any layout.
using KeyCode = int;

// Modifier bits, independent of CGEventFlags.
enum Modifier : std::uint32_t {
    ModShift = 1u << 0,
    ModControl = 1u << 1,
    ModOption = 1u << 2,
    ModCommand = 1u << 3,
};

namespace key {
constexpr KeyCode Return = 0x24;
constexpr KeyCode Tab = 0x30;
constexpr KeyCode Space = 0x31;
constexpr KeyCode Delete = 0x33; // Backspace
constexpr KeyCode Escape = 0x35;
constexpr KeyCode RightCommand = 0x36;
constexpr KeyCode Command = 0x37;
constexpr KeyCode Shift = 0x38;
constexpr KeyCode CapsLock = 0x39;
constexpr KeyCode Option = 0x3A;
constexpr KeyCode Control = 0x3B;
constexpr KeyCode RightShift = 0x3C;
constexpr KeyCode RightOption = 0x3D;
constexpr KeyCode RightControl = 0x3E;
constexpr KeyCode Function = 0x3F;
constexpr KeyCode F18 = 0x4F;
constexpr KeyCode V = 0x09;
constexpr KeyCode Q = 0x0C;
} // namespace key

// Name of a physical key on a US ANSI keyboard: "a", "7", ";", "space",
// "left", "f5". Empty for keys that can't be bound.
std::string keyName(KeyCode code);
std::optional<KeyCode> keyCodeFromName(std::string_view name);

// Text for a key cap: "A", "7", ";", "Space", "←", "F5".
std::string keyLabel(std::string_view name);

// Modifier keys (Shift, Option, ...) and Caps Lock.
bool isModifierKey(KeyCode code);
// The modifier bit a modifier key sets, 0 for other keys.
std::uint32_t modifierBit(KeyCode code);

// What a user typed into the key field -> key name. Accepts Latin letters,
// digits and punctuation, and Cyrillic letters (mapped to the physical key
// they share in ЙЦУКЕН), so "е" and "t" both mean the T key.
std::string normalizeKeyInput(std::string_view utf8);

} // namespace leap
