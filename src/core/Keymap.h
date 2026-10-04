#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace leap {

enum class ActionType {
    None,
    App,    // value: path to the .app bundle
    Open,   // value: file or folder path (~ allowed)
    Url,    // value: URL
    Shell,  // value: zsh command line
    Text,   // value: text to paste into the front app
    Window, // value: window command id (see windowCommands())
    System, // value: system command id (see systemCommands())
};

std::string_view actionTypeId(ActionType type);
ActionType actionTypeFromId(std::string_view id);

struct Action {
    ActionType type = ActionType::None;
    std::string value;

    bool operator==(const Action&) const = default;
};

// One key in the tree: a group of further keys or a leaf with an action.
struct Node {
    std::string key;   // key name, see keyName()
    std::string title; // shown in the overlay
    bool group = false;
    // Group only: stay open after running an action, so e.g. "volume up"
    // can be pressed repeatedly.
    bool sticky = false;
    Action action;              // leaf only
    std::vector<Node> children; // group only

    bool operator==(const Node&) const = default;
};

// Indices from the root down to a node; empty = the root.
using Path = std::vector<int>;

const Node* nodeAt(const Node& root, const Path& path);
Node* nodeAt(Node& root, const Path& path);

// Moves the node at `from` into the group at `toParent`, before the child
// that is at `toIndex` now (indices as before the move; past the end =
// append). Returns the node's new path, or nullopt if the move is invalid:
// bad paths, `toParent` not a group, or a group dropped into itself.
std::optional<Path> moveNode(Node& root, const Path& from, const Path& toParent, int toIndex);

// The child of `group` bound to `keyName`, or -1.
int findChild(const Node& group, std::string_view keyName);

// Keys bound twice inside the same group (only the first one is reachable).
struct Conflict {
    Path group;
    std::string key;
};
std::vector<Conflict> findConflicts(const Node& root);

// A key name not used yet in `group`, preferring the letters of `title`.
std::string suggestKey(const Node& group, std::string_view title);

struct CommandInfo {
    const char* id;
    const char* title; // Russian UI title
};
const std::vector<CommandInfo>& windowCommands();
const std::vector<CommandInfo>& systemCommands();
std::string commandTitle(ActionType type, std::string_view id);

} // namespace leap
