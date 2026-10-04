#include "Sequencer.h"

namespace leap {

void Sequencer::setRoot(Node root)
{
    m_root = std::move(root);
    m_root.group = true;
    m_path.clear();
    m_active = false;
}

void Sequencer::activate()
{
    m_path.clear();
    m_active = true;
}

void Sequencer::cancel()
{
    m_path.clear();
    m_active = false;
}

const Node& Sequencer::current() const
{
    const Node* node = nodeAt(m_root, m_path);
    return node ? *node : m_root;
}

std::vector<std::string> Sequencer::breadcrumb() const
{
    std::vector<std::string> out;
    const Node* node = &m_root;
    for (const int index : m_path) {
        node = &node->children[static_cast<size_t>(index)];
        out.push_back(node->title);
    }
    return out;
}

Sequencer::Outcome Sequencer::press(KeyCode code)
{
    if (!m_active)
        return {};

    if (code == key::Escape) {
        cancel();
        return { Result::Cancelled, {}, {} };
    }
    if (code == key::Delete) {
        if (m_path.empty()) {
            cancel();
            return { Result::Cancelled, {}, {} };
        }
        m_path.pop_back();
        return { Result::Back, {}, {} };
    }

    const std::string name = keyName(code);
    const Node& group = current();
    const int index = name.empty() ? -1 : findChild(group, name);
    if (index < 0)
        return { Result::Unknown, {}, {} };

    const Node& child = group.children[static_cast<size_t>(index)];
    if (child.group) {
        m_path.push_back(index);
        return { Result::Descended, {}, {} };
    }

    Outcome out { Result::Executed, child.action, child.title };
    if (!group.sticky)
        cancel();
    return out;
}

} // namespace leap
