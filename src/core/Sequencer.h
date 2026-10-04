#pragma once

#include "Keymap.h"
#include "Keys.h"

#include <string>
#include <vector>

namespace leap {

// Walks the key tree one key at a time once the leader key was pressed.
class Sequencer {
public:
    enum class Result {
        Inactive,  // press() while not active
        Descended, // opened a group
        Back,      // Backspace: one level up
        Cancelled, // Esc, Backspace at the top, or the leader again
        Executed,  // ran a leaf (see Outcome::action)
        Unknown,   // no such key here; still active
    };
    struct Outcome {
        Result result = Result::Inactive;
        Action action;     // Executed
        std::string title; // Executed: the leaf's title
    };

    void setRoot(Node root);
    const Node& root() const { return m_root; }

    bool active() const { return m_active; }
    void activate();
    void cancel();
    Outcome press(KeyCode code);

    const Path& path() const { return m_path; }
    // The group whose keys are on screen now.
    const Node& current() const;
    // Titles of the groups opened so far (without the root).
    std::vector<std::string> breadcrumb() const;

private:
    Node m_root;
    Path m_path;
    bool m_active = false;
};

} // namespace leap
