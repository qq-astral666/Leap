#include "HidMapping.h"

#include <algorithm>
#include <cctype>
#include <optional>

namespace leap {

namespace {

// Value of `key = <number>;` inside one "{ ... }" block. hidutil prints
// decimals, but accept 0x too.
std::optional<std::uint64_t> field(std::string_view block, std::string_view key)
{
    const auto at = block.find(key);
    if (at == std::string_view::npos)
        return std::nullopt;
    auto rest = block.substr(at + key.size());
    const auto eq = rest.find('=');
    if (eq == std::string_view::npos)
        return std::nullopt;
    rest.remove_prefix(eq + 1);
    while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.front())))
        rest.remove_prefix(1);
    int base = 10;
    if (rest.starts_with("0x") || rest.starts_with("0X")) {
        base = 16;
        rest.remove_prefix(2);
    }
    std::uint64_t value = 0;
    size_t digits = 0;
    for (const char c : rest) {
        int d = -1;
        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (base == 16 && std::isxdigit(static_cast<unsigned char>(c)))
            d = std::tolower(static_cast<unsigned char>(c)) - 'a' + 10;
        if (d < 0)
            break;
        value = value * static_cast<std::uint64_t>(base) + static_cast<std::uint64_t>(d);
        ++digits;
    }
    if (digits == 0)
        return std::nullopt;
    return value;
}

} // namespace

std::vector<HidMapping> parseHidutilMappings(std::string_view out)
{
    std::vector<HidMapping> list;
    size_t pos = 0;
    while (true) {
        const auto open = out.find('{', pos);
        if (open == std::string_view::npos)
            break;
        const auto close = out.find('}', open);
        if (close == std::string_view::npos)
            break;
        const auto block = out.substr(open, close - open);
        const auto src = field(block, "HIDKeyboardModifierMappingSrc");
        const auto dst = field(block, "HIDKeyboardModifierMappingDst");
        if (src && dst)
            list.push_back({ *src, *dst });
        pos = close + 1;
    }
    return list;
}

std::vector<HidMapping> withCapsLockRemap(std::vector<HidMapping> list, bool enable)
{
    // Enabling overrides whatever Caps Lock was mapped to; disabling removes
    // only Leap's own mapping.
    std::erase_if(list, [enable](const HidMapping& m) {
        return m.src == kHidCapsLock && (enable || m.dst == kHidF18);
    });
    if (enable)
        list.push_back({ kHidCapsLock, kHidF18 });
    return list;
}

std::string hidutilSetJson(const std::vector<HidMapping>& list)
{
    std::string json = "{\"UserKeyMapping\":[";
    for (size_t i = 0; i < list.size(); ++i) {
        if (i)
            json += ",";
        json += "{\"HIDKeyboardModifierMappingSrc\":" + std::to_string(list[i].src)
            + ",\"HIDKeyboardModifierMappingDst\":" + std::to_string(list[i].dst) + "}";
    }
    return json + "]}";
}

} // namespace leap
