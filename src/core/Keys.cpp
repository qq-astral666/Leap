#include "Keys.h"

#include <array>
#include <cctype>
#include <utility>

namespace leap {

namespace {

struct KeyEntry {
    KeyCode code;
    const char* name;
    const char* label;
};

// kVK_ANSI_* and friends from <HIToolbox/Events.h>.
constexpr std::array kKeys = {
    KeyEntry { 0x00, "a", "A" },
    KeyEntry { 0x01, "s", "S" },
    KeyEntry { 0x02, "d", "D" },
    KeyEntry { 0x03, "f", "F" },
    KeyEntry { 0x04, "h", "H" },
    KeyEntry { 0x05, "g", "G" },
    KeyEntry { 0x06, "z", "Z" },
    KeyEntry { 0x07, "x", "X" },
    KeyEntry { 0x08, "c", "C" },
    KeyEntry { 0x09, "v", "V" },
    KeyEntry { 0x0B, "b", "B" },
    KeyEntry { 0x0C, "q", "Q" },
    KeyEntry { 0x0D, "w", "W" },
    KeyEntry { 0x0E, "e", "E" },
    KeyEntry { 0x0F, "r", "R" },
    KeyEntry { 0x10, "y", "Y" },
    KeyEntry { 0x11, "t", "T" },
    KeyEntry { 0x12, "1", "1" },
    KeyEntry { 0x13, "2", "2" },
    KeyEntry { 0x14, "3", "3" },
    KeyEntry { 0x15, "4", "4" },
    KeyEntry { 0x16, "6", "6" },
    KeyEntry { 0x17, "5", "5" },
    KeyEntry { 0x18, "=", "=" },
    KeyEntry { 0x19, "9", "9" },
    KeyEntry { 0x1A, "7", "7" },
    KeyEntry { 0x1B, "-", "-" },
    KeyEntry { 0x1C, "8", "8" },
    KeyEntry { 0x1D, "0", "0" },
    KeyEntry { 0x1E, "]", "]" },
    KeyEntry { 0x1F, "o", "O" },
    KeyEntry { 0x20, "u", "U" },
    KeyEntry { 0x21, "[", "[" },
    KeyEntry { 0x22, "i", "I" },
    KeyEntry { 0x23, "p", "P" },
    KeyEntry { 0x24, "return", "↩" },
    KeyEntry { 0x25, "l", "L" },
    KeyEntry { 0x26, "j", "J" },
    KeyEntry { 0x27, "'", "'" },
    KeyEntry { 0x28, "k", "K" },
    KeyEntry { 0x29, ";", ";" },
    KeyEntry { 0x2A, "\\", "\\" },
    KeyEntry { 0x2B, ",", "," },
    KeyEntry { 0x2C, "/", "/" },
    KeyEntry { 0x2D, "n", "N" },
    KeyEntry { 0x2E, "m", "M" },
    KeyEntry { 0x2F, ".", "." },
    KeyEntry { 0x30, "tab", "⇥" },
    KeyEntry { 0x31, "space", "Space" },
    KeyEntry { 0x32, "`", "`" },
    KeyEntry { 0x7A, "f1", "F1" },
    KeyEntry { 0x78, "f2", "F2" },
    KeyEntry { 0x63, "f3", "F3" },
    KeyEntry { 0x76, "f4", "F4" },
    KeyEntry { 0x60, "f5", "F5" },
    KeyEntry { 0x61, "f6", "F6" },
    KeyEntry { 0x62, "f7", "F7" },
    KeyEntry { 0x64, "f8", "F8" },
    KeyEntry { 0x65, "f9", "F9" },
    KeyEntry { 0x6D, "f10", "F10" },
    KeyEntry { 0x67, "f11", "F11" },
    KeyEntry { 0x6F, "f12", "F12" },
    KeyEntry { 0x4F, "f18", "F18" },
    KeyEntry { 0x7B, "left", "←" },
    KeyEntry { 0x7C, "right", "→" },
    KeyEntry { 0x7D, "down", "↓" },
    KeyEntry { 0x7E, "up", "↑" },
};

// ЙЦУКЕН letter -> the QWERTY key at the same position.
constexpr std::array<std::pair<const char*, const char*>, 33> kCyrillic = { {
    { "й", "q" }, { "ц", "w" }, { "у", "e" }, { "к", "r" }, { "е", "t" },
    { "н", "y" }, { "г", "u" }, { "ш", "i" }, { "щ", "o" }, { "з", "p" },
    { "х", "[" }, { "ъ", "]" }, { "ф", "a" }, { "ы", "s" }, { "в", "d" },
    { "а", "f" }, { "п", "g" }, { "р", "h" }, { "о", "j" }, { "л", "k" },
    { "д", "l" }, { "ж", ";" }, { "э", "'" }, { "я", "z" }, { "ч", "x" },
    { "с", "c" }, { "м", "v" }, { "и", "b" }, { "т", "n" }, { "ь", "m" },
    { "б", "," }, { "ю", "." }, { "ё", "`" },
} };

// Uppercase Cyrillic (А..Я, Ё) -> lowercase, in UTF-8.
std::string lowerCyrillic(std::string_view s)
{
    if (s.size() != 2)
        return std::string(s);
    const auto b0 = static_cast<unsigned char>(s[0]);
    const auto b1 = static_cast<unsigned char>(s[1]);
    const unsigned cp = ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu);
    unsigned lower = cp;
    if (cp >= 0x410 && cp <= 0x42F)
        lower = cp + 0x20;
    else if (cp == 0x401)
        lower = 0x451;
    std::string out;
    out += static_cast<char>(0xC0 | (lower >> 6));
    out += static_cast<char>(0x80 | (lower & 0x3F));
    return out;
}

} // namespace

std::string keyName(KeyCode code)
{
    for (const auto& k : kKeys)
        if (k.code == code)
            return k.name;
    return {};
}

std::optional<KeyCode> keyCodeFromName(std::string_view name)
{
    for (const auto& k : kKeys)
        if (name == k.name)
            return k.code;
    return std::nullopt;
}

std::string keyLabel(std::string_view name)
{
    for (const auto& k : kKeys)
        if (name == k.name)
            return k.label;
    return std::string(name);
}

bool isModifierKey(KeyCode code)
{
    return code >= key::RightCommand && code <= key::Function;
}

std::uint32_t modifierBit(KeyCode code)
{
    switch (code) {
    case key::Shift:
    case key::RightShift:
        return ModShift;
    case key::Control:
    case key::RightControl:
        return ModControl;
    case key::Option:
    case key::RightOption:
        return ModOption;
    case key::Command:
    case key::RightCommand:
        return ModCommand;
    default:
        return 0;
    }
}

std::string normalizeKeyInput(std::string_view utf8)
{
    // Trim surrounding whitespace, but a lone space is the Space key.
    if (utf8 == " ")
        return "space";
    while (!utf8.empty() && (utf8.front() == ' ' || utf8.front() == '\t'))
        utf8.remove_prefix(1);
    while (!utf8.empty() && (utf8.back() == ' ' || utf8.back() == '\t'))
        utf8.remove_suffix(1);
    if (utf8.empty())
        return {};

    if (utf8.size() == 1) {
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(utf8[0])));
        // Shifted symbols -> their key.
        constexpr std::array<std::pair<char, char>, 20> shifted = { {
            { '!', '1' }, { '@', '2' }, { '#', '3' }, { '$', '4' }, { '%', '5' },
            { '^', '6' }, { '&', '7' }, { '*', '8' }, { '(', '9' }, { ')', '0' },
            { '_', '-' }, { '+', '=' }, { '{', '[' }, { '}', ']' }, { ':', ';' },
            { '"', '\'' }, { '<', ',' }, { '>', '.' }, { '?', '/' }, { '~', '`' },
        } };
        char k = c;
        for (const auto& [from, to] : shifted)
            if (c == from)
                k = to;
        const std::string name(1, k);
        return keyCodeFromName(name) ? name : std::string {};
    }

    const std::string lower = lowerCyrillic(utf8);
    for (const auto& [ru, en] : kCyrillic)
        if (lower == ru)
            return en;

    // Already a key name ("space", "f5", "left")?
    std::string ascii;
    for (const char ch : utf8)
        ascii += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return keyCodeFromName(ascii) ? ascii : std::string {};
}

} // namespace leap
