#include "Engine.h"

#include <array>
#include <utility>

namespace leap {

namespace {

constexpr std::array<std::pair<KeyCode, const char*>, 4> kTapKeys = { {
    { key::RightCommand, "right_command" },
    { key::RightOption, "right_option" },
    { key::RightControl, "right_control" },
    { key::RightShift, "right_shift" },
} };

constexpr std::array<std::pair<std::uint32_t, const char*>, 4> kModNames = { {
    { ModControl, "control" },
    { ModOption, "option" },
    { ModShift, "shift" },
    { ModCommand, "command" },
} };

} // namespace

std::string Leader::id() const
{
    switch (kind) {
    case Kind::CapsLock:
        return "capslock";
    case Kind::Tap:
        for (const auto& [code, name] : kTapKeys)
            if (code == key)
                return std::string("tap:") + name;
        return "tap:right_command";
    case Kind::Chord: {
        std::string out = "chord:";
        for (const auto& [bit, name] : kModNames)
            if (mods & bit)
                out += std::string(name) + "+";
        return out + keyName(key);
    }
    }
    return "capslock";
}

std::optional<Leader> Leader::fromId(std::string_view id)
{
    if (id == "capslock")
        return capsLock();
    if (id.starts_with("tap:")) {
        id.remove_prefix(4);
        for (const auto& [code, name] : kTapKeys)
            if (id == name)
                return tap(code);
        return std::nullopt;
    }
    if (id.starts_with("chord:")) {
        id.remove_prefix(6);
        std::uint32_t mods = 0;
        while (true) {
            const auto plus = id.find('+');
            if (plus == std::string_view::npos || plus == id.size() - 1)
                break; // the rest is the key ("+" itself can't be a key name)
            const auto part = id.substr(0, plus);
            bool known = false;
            for (const auto& [bit, name] : kModNames)
                if (part == name) {
                    mods |= bit;
                    known = true;
                }
            if (!known)
                return std::nullopt;
            id.remove_prefix(plus + 1);
        }
        const auto code = keyCodeFromName(id);
        // A chord without modifiers would eat that key everywhere.
        if (!code || mods == 0)
            return std::nullopt;
        return chord(*code, mods);
    }
    return std::nullopt;
}

void Engine::setLeader(const Leader& leader)
{
    m_leader = leader;
    m_tapArmed = false;
}

void Engine::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
        m_seq.cancel();
}

bool Engine::isLeaderDown(const KeyEvent& e) const
{
    switch (m_leader.kind) {
    case Leader::Kind::CapsLock:
        return e.code == key::F18; // modifiers don't matter
    case Leader::Kind::Chord:
        return e.code == m_leader.key && e.mods == m_leader.mods;
    case Leader::Kind::Tap:
        return false;
    }
    return false;
}

Engine::Decision Engine::onFlags(const KeyEvent& e)
{
    Decision d; // modifier changes always pass through
    if (!m_enabled || m_leader.kind != Leader::Kind::Tap || e.code != m_leader.key) {
        m_tapArmed = false;
        return d;
    }
    const std::uint32_t bit = modifierBit(e.code);
    const bool pressed = (e.mods & bit) != 0;
    if (pressed) {
        // Only on its own: ⌘ held as part of a shortcut isn't a tap.
        m_tapArmed = e.mods == bit;
        m_tapTime = e.time;
        return d;
    }
    const bool tapped = m_tapArmed && e.time - m_tapTime <= kTapMaxSeconds;
    m_tapArmed = false;
    if (!tapped)
        return d;
    if (m_seq.active()) {
        m_seq.cancel();
        d.effect.kind = Effect::Kind::Cancelled;
    } else {
        m_seq.activate();
        d.effect.kind = Effect::Kind::Activated;
    }
    return d;
}

Engine::Decision Engine::handle(const KeyEvent& e)
{
    switch (e.kind) {
    case KeyEvent::Kind::Flags:
        return onFlags(e);
    case KeyEvent::Kind::Up:
        return { m_swallowedDown.erase(e.code) > 0, {} };
    case KeyEvent::Kind::Down:
        break;
    }

    m_tapArmed = false; // a key went down while the modifier was held
    Decision d;
    if (!m_enabled)
        return d;

    if (!m_seq.active()) {
        if (!isLeaderDown(e))
            return d;
        d.swallow = true;
        m_swallowedDown.insert(e.code);
        if (!e.repeat) {
            m_seq.activate();
            d.effect.kind = Effect::Kind::Activated;
        }
        return d;
    }

    // Active: every key belongs to the sequence...
    if (isLeaderDown(e)) {
        d.swallow = true;
        m_swallowedDown.insert(e.code);
        if (!e.repeat) {
            m_seq.cancel();
            d.effect.kind = Effect::Kind::Cancelled;
        }
        return d;
    }
    // ...except shortcuts: ⌘/⌃ + key means the user moved on.
    if (e.mods & (ModCommand | ModControl)) {
        m_seq.cancel();
        d.effect.kind = Effect::Kind::Cancelled;
        return d;
    }

    d.swallow = true;
    m_swallowedDown.insert(e.code);
    // Holding a key repeats it only inside sticky groups (volume up...).
    if (e.repeat && !m_seq.current().sticky)
        return d;

    const auto out = m_seq.press(e.code);
    switch (out.result) {
    case Sequencer::Result::Descended:
    case Sequencer::Result::Back:
        d.effect.kind = Effect::Kind::Changed;
        break;
    case Sequencer::Result::Cancelled:
        d.effect.kind = Effect::Kind::Cancelled;
        break;
    case Sequencer::Result::Executed:
        d.effect = { Effect::Kind::Executed, out.action, out.title };
        break;
    case Sequencer::Result::Unknown:
        d.effect.kind = Effect::Kind::Unknown;
        break;
    case Sequencer::Result::Inactive:
        break;
    }
    return d;
}

} // namespace leap
