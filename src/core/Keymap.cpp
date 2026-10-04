#include "Keymap.h"

#include "Keys.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace leap {

namespace {

struct TypeId {
    ActionType type;
    const char* id;
};
constexpr TypeId kTypeIds[] = {
    { ActionType::None, "none" },
    { ActionType::App, "app" },
    { ActionType::Open, "open" },
    { ActionType::Url, "url" },
    { ActionType::Shell, "shell" },
    { ActionType::Text, "text" },
    { ActionType::Window, "window" },
    { ActionType::System, "system" },
};

void collectConflicts(const Node& group, Path& path, std::vector<Conflict>& out)
{
    std::set<std::string> seen;
    std::set<std::string> reported;
    for (const auto& child : group.children) {
        if (child.key.empty())
            continue;
        if (!seen.insert(child.key).second && reported.insert(child.key).second)
            out.push_back({ path, child.key });
    }
    for (int i = 0; i < static_cast<int>(group.children.size()); ++i) {
        const Node& child = group.children[static_cast<size_t>(i)];
        if (!child.group)
            continue;
        path.push_back(i);
        collectConflicts(child, path, out);
        path.pop_back();
    }
}

// Splits UTF-8 into code point substrings.
std::vector<std::string> utf8Chars(std::string_view s)
{
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size();) {
        const auto b = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if (b >= 0xF0)
            len = 4;
        else if (b >= 0xE0)
            len = 3;
        else if (b >= 0xC0)
            len = 2;
        out.emplace_back(s.substr(i, len));
        i += len;
    }
    return out;
}

} // namespace

std::string_view actionTypeId(ActionType type)
{
    for (const auto& t : kTypeIds)
        if (t.type == type)
            return t.id;
    return "none";
}

ActionType actionTypeFromId(std::string_view id)
{
    for (const auto& t : kTypeIds)
        if (id == t.id)
            return t.type;
    return ActionType::None;
}

const Node* nodeAt(const Node& root, const Path& path)
{
    const Node* node = &root;
    for (const int index : path) {
        if (index < 0 || index >= static_cast<int>(node->children.size()))
            return nullptr;
        node = &node->children[static_cast<size_t>(index)];
    }
    return node;
}

Node* nodeAt(Node& root, const Path& path)
{
    return const_cast<Node*>(nodeAt(static_cast<const Node&>(root), path));
}

std::optional<Path> moveNode(Node& root, const Path& from, const Path& toParent, int toIndex)
{
    if (from.empty())
        return std::nullopt;
    const Path fromParent(from.begin(), from.end() - 1);
    const int fromIndex = from.back();
    Node* source = nodeAt(root, fromParent);
    Node* target = nodeAt(root, toParent);
    if (!source || !target || !target->group || fromIndex < 0
        || fromIndex >= static_cast<int>(source->children.size()))
        return std::nullopt;
    // Into itself or its own descendants.
    if (toParent.size() >= from.size() && std::equal(from.begin(), from.end(), toParent.begin()))
        return std::nullopt;

    // Paths after removing the node: siblings behind it shift up by one.
    Path parent = toParent;
    int index = std::max(0, toIndex);
    if (parent.size() > fromParent.size() && std::equal(fromParent.begin(), fromParent.end(), parent.begin())
        && parent[fromParent.size()] > fromIndex)
        --parent[fromParent.size()];
    if (parent == fromParent && index > fromIndex)
        --index;

    Node moved = std::move(source->children[static_cast<size_t>(fromIndex)]);
    source->children.erase(source->children.begin() + fromIndex);
    Node* dest = nodeAt(root, parent); // re-resolve: the vectors changed
    index = std::min(index, static_cast<int>(dest->children.size()));
    dest->children.insert(dest->children.begin() + index, std::move(moved));
    parent.push_back(index);
    return parent;
}

int findChild(const Node& group, std::string_view keyName)
{
    for (int i = 0; i < static_cast<int>(group.children.size()); ++i)
        if (group.children[static_cast<size_t>(i)].key == keyName)
            return i;
    return -1;
}

std::vector<Conflict> findConflicts(const Node& root)
{
    std::vector<Conflict> out;
    Path path;
    collectConflicts(root, path, out);
    return out;
}

std::string suggestKey(const Node& group, std::string_view title)
{
    auto free = [&](const std::string& k) { return !k.empty() && findChild(group, k) < 0; };
    for (const auto& ch : utf8Chars(title)) {
        // Letters (Latin or Cyrillic) and digits of the title, not "!" -> 1.
        if (ch.size() == 1 && !std::isalnum(static_cast<unsigned char>(ch[0])))
            continue;
        const std::string k = normalizeKeyInput(ch);
        // Letters and digits only: punctuation from a title makes a poor key.
        if (k.size() == 1 && std::isalnum(static_cast<unsigned char>(k[0])) && free(k))
            return k;
    }
    for (const char* c = "abcdefghijklmnopqrstuvwxyz1234567890"; *c; ++c)
        if (free(std::string(1, *c)))
            return std::string(1, *c);
    return {};
}

const std::vector<CommandInfo>& windowCommands()
{
    static const std::vector<CommandInfo> list = {
        { "left-half", "Левая половина" },
        { "right-half", "Правая половина" },
        { "top-half", "Верхняя половина" },
        { "bottom-half", "Нижняя половина" },
        { "maximize", "Развернуть" },
        { "almost-maximize", "Почти на весь экран" },
        { "center", "По центру" },
        { "left-third", "Левая треть" },
        { "center-third", "Средняя треть" },
        { "right-third", "Правая треть" },
        { "left-two-thirds", "Левые две трети" },
        { "right-two-thirds", "Правые две трети" },
        { "top-left", "Левый верхний угол" },
        { "top-right", "Правый верхний угол" },
        { "bottom-left", "Левый нижний угол" },
        { "bottom-right", "Правый нижний угол" },
        { "next-display", "На следующий монитор" },
    };
    return list;
}

const std::vector<CommandInfo>& systemCommands()
{
    static const std::vector<CommandInfo> list = {
        { "volume-up", "Громче" },
        { "volume-down", "Тише" },
        { "mute", "Выключить звук" },
        { "dark-mode", "Тёмная тема" },
        { "sleep-display", "Выключить экран" },
        { "lock", "Заблокировать" },
    };
    return list;
}

std::string commandTitle(ActionType type, std::string_view id)
{
    const auto* list = type == ActionType::Window ? &windowCommands()
        : type == ActionType::System             ? &systemCommands()
                                                 : nullptr;
    if (list)
        for (const auto& c : *list)
            if (id == c.id)
                return c.title;
    return {};
}

} // namespace leap
