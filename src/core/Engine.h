#pragma once

#include "Keymap.h"
#include "Keys.h"
#include "Sequencer.h"

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace leap {

struct KeyEvent {
    enum class Kind { Down, Up, Flags };
    Kind kind = Kind::Down;
    KeyCode code = 0;
    std::uint32_t mods = 0; // Modifier bits after this event
    bool repeat = false;    // key auto-repeat
    double time = 0;        // seconds, any monotonic clock
};

// What starts a sequence.
struct Leader {
    enum class Kind {
        CapsLock, // Caps Lock remapped to F18 (see HidMapping.h)
        Tap,      // a modifier key pressed and released on its own
        Chord,    // modifiers + key, e.g. ⌥Space
    };
    Kind kind = Kind::CapsLock;
    KeyCode key = key::F18;
    std::uint32_t mods = 0; // Chord only

    static Leader capsLock() { return {}; }
    static Leader tap(KeyCode modifierKey) { return { Kind::Tap, modifierKey, 0 }; }
    static Leader chord(KeyCode k, std::uint32_t m) { return { Kind::Chord, k, m }; }

    // "capslock", "tap:right_command", "chord:option+space"
    std::string id() const;
    static std::optional<Leader> fromId(std::string_view id);

    bool operator==(const Leader&) const = default;
};

// Longest press of a tap leader that still counts as a tap.
constexpr double kTapMaxSeconds = 0.4;

// Decides, for every keyboard event in the system, whether it starts or
// continues a sequence and must be swallowed. Pure logic: the platform feeds
// it events from a CGEventTap and acts on the result.
class Engine {
public:
    struct Effect {
        enum class Kind {
            None,
            Activated, // leader pressed: show the root
            Changed,   // went into a group or back
            Cancelled,
            Executed,  // run `action`; still active() if the group is sticky
            Unknown,   // pressed a key that isn't bound here
        };
        Kind kind = Kind::None;
        Action action;
        std::string title;
    };
    struct Decision {
        bool swallow = false;
        Effect effect;
    };

    void setLeader(const Leader& leader);
    const Leader& leader() const { return m_leader; }
    void setRoot(Node root) { m_seq.setRoot(std::move(root)); }
    void setEnabled(bool enabled);
    bool enabled() const { return m_enabled; }

    Decision handle(const KeyEvent& e);
    // Timeout, or the overlay was dismissed some other way.
    void cancel() { m_seq.cancel(); }

    const Sequencer& sequencer() const { return m_seq; }
    bool active() const { return m_seq.active(); }

private:
    bool isLeaderDown(const KeyEvent& e) const;
    Decision onFlags(const KeyEvent& e);

    Leader m_leader;
    Sequencer m_seq;
    bool m_enabled = true;
    std::set<KeyCode> m_swallowedDown; // their key-up is swallowed too
    bool m_tapArmed = false;
    double m_tapTime = 0;
};

} // namespace leap
