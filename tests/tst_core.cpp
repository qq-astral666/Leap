// Unit tests for Leap's core: keys, the key tree, sequences, the leader key,
// window layouts and the Caps Lock remap. Dependency-free: plain C++20.

#include "Engine.h"
#include "HidMapping.h"
#include "Keymap.h"
#include "Keys.h"
#include "Layout.h"
#include "Sequencer.h"

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

using namespace leap;

namespace {

int g_failed = 0;
int g_checks = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failed;                                                               \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);             \
        }                                                                             \
    } while (0)

struct Test {
    const char* name;
    std::function<void()> fn;
};
std::vector<Test>& registry()
{
    static std::vector<Test> tests;
    return tests;
}
struct Register {
    Register(const char* name, std::function<void()> fn) { registry().push_back({ name, std::move(fn) }); }
};
#define TEST(name)                                  \
    static void name();                             \
    static const Register reg_##name(#name, name);  \
    static void name()

KeyCode K(const char* name) { return *keyCodeFromName(name); }

Node leaf(const char* k, const char* title, ActionType type, const char* value)
{
    Node n;
    n.key = k;
    n.title = title;
    n.action = { type, value };
    return n;
}

Node group(const char* k, const char* title, std::vector<Node> children, bool sticky = false)
{
    Node n;
    n.key = k;
    n.title = title;
    n.group = true;
    n.sticky = sticky;
    n.children = std::move(children);
    return n;
}

Node sampleTree()
{
    Node root;
    root.group = true;
    root.children = {
        leaf("t", "Telegram", ActionType::App, "/Applications/Telegram.app"),
        group("w", "Окна", {
            leaf("h", "Левая половина", ActionType::Window, "left-half"),
            leaf("l", "Правая половина", ActionType::Window, "right-half"),
        }),
        group(";", "Система", {
            leaf("u", "Громче", ActionType::System, "volume-up"),
        }, true),
    };
    return root;
}

KeyEvent down(KeyCode code, std::uint32_t mods = 0, bool repeat = false)
{
    return { KeyEvent::Kind::Down, code, mods, repeat, 0 };
}
KeyEvent up(KeyCode code, std::uint32_t mods = 0) { return { KeyEvent::Kind::Up, code, mods, false, 0 }; }
KeyEvent flags(KeyCode code, std::uint32_t mods, double t) { return { KeyEvent::Kind::Flags, code, mods, false, t }; }

using EK = Engine::Effect::Kind;

// ---------------------------------------------------------------- keys

TEST(key_names_round_trip)
{
    CHECK(keyName(0x11) == "t");
    CHECK(keyName(0x31) == "space");
    CHECK(keyName(key::Escape).empty());
    CHECK(keyName(key::CapsLock).empty());
    for (int code = 0; code < 128; ++code) {
        const auto name = keyName(code);
        if (!name.empty())
            CHECK(keyCodeFromName(name) == code);
    }
    CHECK(keyLabel("t") == "T");
    CHECK(keyLabel("left") == "←");
}

TEST(modifier_keys)
{
    CHECK(isModifierKey(key::RightCommand));
    CHECK(isModifierKey(key::CapsLock));
    CHECK(!isModifierKey(K("a")));
    CHECK(modifierBit(key::RightOption) == ModOption);
    CHECK(modifierBit(key::Command) == ModCommand);
    CHECK(modifierBit(K("a")) == 0);
}

TEST(key_input_is_layout_independent)
{
    CHECK(normalizeKeyInput("t") == "t");
    CHECK(normalizeKeyInput("T") == "t");
    CHECK(normalizeKeyInput("е") == "t"); // Cyrillic е sits on T
    CHECK(normalizeKeyInput("Е") == "t");
    CHECK(normalizeKeyInput("ж") == ";");
    CHECK(normalizeKeyInput("ё") == "`");
    CHECK(normalizeKeyInput("Ё") == "`");
    CHECK(normalizeKeyInput(":") == ";");
    CHECK(normalizeKeyInput("!") == "1");
    CHECK(normalizeKeyInput(" ") == "space");
    CHECK(normalizeKeyInput(" a ") == "a");
    CHECK(normalizeKeyInput("F5") == "f5");
    CHECK(normalizeKeyInput("").empty());
    CHECK(normalizeKeyInput("ab").empty());
    CHECK(normalizeKeyInput("😀").empty());
}

// ---------------------------------------------------------------- keymap

TEST(action_type_ids)
{
    for (auto t : { ActionType::App, ActionType::Open, ActionType::Url, ActionType::Shell,
                    ActionType::Text, ActionType::Window, ActionType::System })
        CHECK(actionTypeFromId(actionTypeId(t)) == t);
    CHECK(actionTypeFromId("bogus") == ActionType::None);
}

TEST(node_paths)
{
    Node root = sampleTree();
    CHECK(nodeAt(root, {}) == &root);
    CHECK(nodeAt(root, { 1, 0 })->title == "Левая половина");
    CHECK(nodeAt(root, { 5 }) == nullptr);
    CHECK(nodeAt(root, { 0, 0 }) == nullptr);
    CHECK(findChild(root, "w") == 1);
    CHECK(findChild(root, "z") == -1);
}

TEST(move_nodes)
{
    // root: t, w{h,l}, ;{u}
    Node root = sampleTree();
    auto titles = [](const Node& g) {
        std::vector<std::string> out;
        for (const auto& c : g.children)
            out.push_back(c.title);
        return out;
    };

    // down within the root: Telegram after Окна
    Node r = root;
    auto p = moveNode(r, { 0 }, {}, 2);
    CHECK(p == Path { 1 });
    CHECK(titles(r) == (std::vector<std::string> { "Окна", "Telegram", "Система" }));

    // up within the root, and append past the end
    r = root;
    CHECK(moveNode(r, { 2 }, {}, 0) == Path { 0 });
    CHECK(titles(r)[0] == "Система");
    r = root;
    CHECK(moveNode(r, { 0 }, {}, 99) == Path { 2 });
    CHECK(titles(r).back() == "Telegram");

    // no-op: before itself / before the next one
    r = root;
    CHECK(moveNode(r, { 1 }, {}, 1) == Path { 1 });
    CHECK(moveNode(r, { 1 }, {}, 2) == Path { 1 });
    CHECK(r == root);

    // into a group that comes after it: the group's index shifts
    r = root;
    p = moveNode(r, { 0 }, { 1 }, 0);
    CHECK(p == (Path { 0, 0 }));
    CHECK(nodeAt(r, *p)->title == "Telegram");
    CHECK(titles(*nodeAt(r, { 0 })) == (std::vector<std::string> { "Telegram", "Левая половина", "Правая половина" }));

    // out of a group to the root
    r = root;
    p = moveNode(r, { 1, 1 }, {}, 0);
    CHECK(p == Path { 0 });
    CHECK(nodeAt(r, { 0 })->title == "Правая половина");
    CHECK(nodeAt(r, { 2 })->children.size() == 1);

    // between groups
    r = root;
    p = moveNode(r, { 2, 0 }, { 1 }, 1);
    CHECK(p == (Path { 1, 1 }));
    CHECK(nodeAt(r, { 2 })->children.empty());

    // invalid
    r = root;
    CHECK(!moveNode(r, { 1 }, { 1 }, 0));    // into itself
    CHECK(!moveNode(r, {}, {}, 0));          // the root
    CHECK(!moveNode(r, { 5 }, {}, 0));       // no such node
    CHECK(!moveNode(r, { 1, 0 }, { 0 }, 0)); // Telegram is not a group
    CHECK(r == root);
}

TEST(conflicts_per_group)
{
    Node root = sampleTree();
    CHECK(findConflicts(root).empty());
    root.children.push_back(leaf("t", "Terminal", ActionType::App, "/x"));
    root.children.push_back(leaf("t", "Third", ActionType::App, "/y"));
    nodeAt(root, { 1 })->children.push_back(leaf("h", "Dup", ActionType::Window, "maximize"));
    // "h" in another group is fine
    root.children.push_back(leaf("h", "Help", ActionType::Url, "https://x"));
    const auto c = findConflicts(root);
    CHECK(c.size() == 2);
    CHECK(c[0].group.empty() && c[0].key == "t");
    CHECK(c[1].group == Path { 1 } && c[1].key == "h");
}

TEST(suggest_key)
{
    const Node root = sampleTree();
    CHECK(suggestKey(root, "Terminal") == "e"); // t is taken
    CHECK(suggestKey(root, "Почта") == "g");    // п sits on G
    CHECK(suggestKey(root, "") == "a");
    CHECK(suggestKey(root, "!!!") == "a");
}

TEST(command_titles)
{
    CHECK(commandTitle(ActionType::Window, "maximize") == "Развернуть");
    CHECK(commandTitle(ActionType::System, "lock") == "Заблокировать");
    CHECK(commandTitle(ActionType::Window, "lock").empty());
    CHECK(windowCommands().size() > 10);
}

// ---------------------------------------------------------------- sequencer

TEST(sequencer_leaf_at_root)
{
    Sequencer s;
    s.setRoot(sampleTree());
    CHECK(s.press(K("t")).result == Sequencer::Result::Inactive);
    s.activate();
    const auto out = s.press(K("t"));
    CHECK(out.result == Sequencer::Result::Executed);
    CHECK(out.action.type == ActionType::App);
    CHECK(out.title == "Telegram");
    CHECK(!s.active());
}

TEST(sequencer_groups_and_back)
{
    Sequencer s;
    s.setRoot(sampleTree());
    s.activate();
    CHECK(s.press(K("w")).result == Sequencer::Result::Descended);
    CHECK(s.current().title == "Окна");
    CHECK(s.breadcrumb() == std::vector<std::string> { "Окна" });
    CHECK(s.press(K("t")).result == Sequencer::Result::Unknown);
    CHECK(s.active());
    CHECK(s.press(key::Delete).result == Sequencer::Result::Back);
    CHECK(s.path().empty());
    CHECK(s.press(key::Delete).result == Sequencer::Result::Cancelled);
    CHECK(!s.active());

    s.activate();
    s.press(K("w"));
    const auto out = s.press(K("l"));
    CHECK(out.result == Sequencer::Result::Executed);
    CHECK(out.action.value == "right-half");
    CHECK(!s.active());
}

TEST(sequencer_escape_and_sticky)
{
    Sequencer s;
    s.setRoot(sampleTree());
    s.activate();
    CHECK(s.press(key::Escape).result == Sequencer::Result::Cancelled);
    CHECK(!s.active());

    s.activate();
    s.press(K(";"));
    CHECK(s.press(K("u")).result == Sequencer::Result::Executed);
    CHECK(s.active()); // sticky group stays open
    CHECK(s.press(K("u")).result == Sequencer::Result::Executed);
    CHECK(s.current().title == "Система");
}

TEST(sequencer_activate_resets_path)
{
    Sequencer s;
    s.setRoot(sampleTree());
    s.activate();
    s.press(K("w"));
    s.activate();
    CHECK(s.path().empty());
}

// ---------------------------------------------------------------- leader

TEST(leader_ids)
{
    for (const auto& l : { Leader::capsLock(), Leader::tap(key::RightCommand), Leader::tap(key::RightOption),
                           Leader::chord(K("space"), ModOption), Leader::chord(K(";"), ModControl | ModShift) }) {
        const auto parsed = Leader::fromId(l.id());
        CHECK(parsed && *parsed == l);
    }
    CHECK(Leader::chord(K("space"), ModOption).id() == "chord:option+space");
    CHECK(Leader::chord(K("k"), ModControl | ModCommand).id() == "chord:control+command+k");
    CHECK(!Leader::fromId("chord:space"));       // no modifiers
    CHECK(!Leader::fromId("chord:hyper+space")); // unknown modifier
    CHECK(!Leader::fromId("tap:left_foot"));
    CHECK(!Leader::fromId(""));
}

TEST(engine_capslock_flow)
{
    Engine e;
    e.setRoot(sampleTree());
    // ordinary typing passes through
    CHECK(!e.handle(down(K("t"))).swallow);
    CHECK(!e.handle(up(K("t"))).swallow);

    auto d = e.handle(down(key::F18));
    CHECK(d.swallow && d.effect.kind == EK::Activated);
    CHECK(e.handle(up(key::F18)).swallow);

    d = e.handle(down(K("w")));
    CHECK(d.swallow && d.effect.kind == EK::Changed);
    CHECK(e.handle(up(K("w"))).swallow);

    d = e.handle(down(K("h")));
    CHECK(d.swallow && d.effect.kind == EK::Executed);
    CHECK(d.effect.action.value == "left-half");
    CHECK(!e.active());
    CHECK(e.handle(up(K("h"))).swallow); // its key-up too
    CHECK(!e.handle(down(K("h"))).swallow);
}

TEST(engine_leader_again_cancels)
{
    Engine e;
    e.setRoot(sampleTree());
    e.handle(down(key::F18));
    e.handle(up(key::F18));
    const auto d = e.handle(down(key::F18));
    CHECK(d.swallow && d.effect.kind == EK::Cancelled);
    CHECK(!e.active());
}

TEST(engine_shortcut_cancels_and_passes)
{
    Engine e;
    e.setRoot(sampleTree());
    e.handle(down(key::F18));
    const auto d = e.handle(down(K("c"), ModCommand));
    CHECK(!d.swallow);
    CHECK(d.effect.kind == EK::Cancelled);
    CHECK(!e.active());
    // Shift is just a key in the sequence
    e.handle(down(key::F18));
    CHECK(e.handle(down(K("t"), ModShift)).effect.kind == EK::Executed);
}

TEST(engine_unknown_and_repeat)
{
    Engine e;
    e.setRoot(sampleTree());
    e.handle(down(key::F18));
    auto d = e.handle(down(K("z")));
    CHECK(d.swallow && d.effect.kind == EK::Unknown);
    CHECK(e.active());
    // holding the leader doesn't toggle
    d = e.handle(down(key::F18, 0, true));
    CHECK(d.swallow && d.effect.kind == EK::None && e.active());
    // auto-repeat outside sticky groups does nothing
    e.handle(down(K("w")));
    d = e.handle(down(K("w"), 0, true));
    CHECK(d.swallow && d.effect.kind == EK::None);
    CHECK(e.sequencer().current().title == "Окна");
}

TEST(engine_sticky_repeat)
{
    Engine e;
    e.setRoot(sampleTree());
    e.handle(down(key::F18));
    e.handle(down(K(";")));
    CHECK(e.handle(down(K("u"))).effect.kind == EK::Executed);
    CHECK(e.handle(down(K("u"), 0, true)).effect.kind == EK::Executed);
    CHECK(e.active());
    CHECK(e.handle(down(key::Escape)).effect.kind == EK::Cancelled);
}

TEST(engine_chord_leader)
{
    Engine e;
    e.setRoot(sampleTree());
    e.setLeader(Leader::chord(K("space"), ModOption));
    CHECK(!e.handle(down(K("space"))).swallow);
    CHECK(!e.handle(down(K("space"), ModOption | ModShift)).swallow);
    CHECK(!e.handle(down(key::F18)).swallow);
    const auto d = e.handle(down(K("space"), ModOption));
    CHECK(d.swallow && d.effect.kind == EK::Activated);
    // option still held while picking: keys still match by position
    CHECK(e.handle(down(K("t"), ModOption)).effect.kind == EK::Executed);
}

TEST(engine_tap_leader)
{
    Engine e;
    e.setRoot(sampleTree());
    e.setLeader(Leader::tap(key::RightCommand));

    // quick tap of right ⌘
    CHECK(!e.handle(flags(key::RightCommand, ModCommand, 1.0)).swallow);
    auto d = e.handle(flags(key::RightCommand, 0, 1.15));
    CHECK(!d.swallow && d.effect.kind == EK::Activated);
    CHECK(e.handle(down(K("t"))).effect.kind == EK::Executed);

    // held too long
    e.handle(flags(key::RightCommand, ModCommand, 2.0));
    CHECK(e.handle(flags(key::RightCommand, 0, 3.0)).effect.kind == EK::None);

    // used as a modifier: ⌘C
    e.handle(flags(key::RightCommand, ModCommand, 4.0));
    CHECK(!e.handle(down(K("c"), ModCommand)).swallow);
    CHECK(e.handle(flags(key::RightCommand, 0, 4.1)).effect.kind == EK::None);

    // pressed together with another modifier
    e.handle(flags(key::Shift, ModShift, 5.0));
    e.handle(flags(key::RightCommand, ModShift | ModCommand, 5.01));
    CHECK(e.handle(flags(key::RightCommand, ModShift, 5.1)).effect.kind == EK::None);
    e.handle(flags(key::Shift, 0, 5.2));

    // left ⌘ isn't the leader
    e.handle(flags(key::Command, ModCommand, 6.0));
    CHECK(e.handle(flags(key::Command, 0, 6.1)).effect.kind == EK::None);

    // second tap closes
    e.handle(flags(key::RightCommand, ModCommand, 7.0));
    e.handle(flags(key::RightCommand, 0, 7.1));
    CHECK(e.active());
    e.handle(flags(key::RightCommand, ModCommand, 7.3));
    CHECK(e.handle(flags(key::RightCommand, 0, 7.4)).effect.kind == EK::Cancelled);
    CHECK(!e.active());
}

TEST(engine_disabled)
{
    Engine e;
    e.setRoot(sampleTree());
    e.handle(down(key::F18));
    e.setEnabled(false);
    CHECK(!e.active());
    CHECK(!e.handle(down(key::F18)).swallow);
    e.setEnabled(true);
    CHECK(e.handle(down(key::F18)).effect.kind == EK::Activated);
}

// ---------------------------------------------------------------- layout

TEST(layout_halves_and_thirds)
{
    const Rect screen { 0, 25, 1440, 875 };
    const Rect win { 100, 100, 800, 600 };
    CHECK(*layoutWindow("left-half", screen, win) == (Rect { 0, 25, 720, 875 }));
    CHECK(*layoutWindow("right-half", screen, win) == (Rect { 720, 25, 720, 875 }));
    CHECK(*layoutWindow("maximize", screen, win) == screen);
    CHECK(*layoutWindow("center-third", screen, win) == (Rect { 480, 25, 480, 875 }));
    CHECK(*layoutWindow("right-two-thirds", screen, win) == (Rect { 480, 25, 960, 875 }));
    CHECK(layoutWindow("bottom-right", screen, win)->x == 720);
    CHECK(!layoutWindow("next-display", screen, win));
    CHECK(!layoutWindow("nope", screen, win));
}

TEST(layout_gap_and_center)
{
    const Rect screen { 0, 0, 1000, 800 };
    const auto l = *layoutWindow("left-half", screen, {}, 10);
    const auto r = *layoutWindow("right-half", screen, {}, 10);
    CHECK(l.x == 10 && l.y == 10 && l.h == 780);
    CHECK(r.right() == 990);
    CHECK(r.x - l.right() == 10);

    const auto c = *layoutWindow("center", screen, { 0, 0, 400, 300 });
    CHECK(c == (Rect { 300, 250, 400, 300 }));
    // bigger than the screen: shrinks to fit
    const auto big = *layoutWindow("center", screen, { 0, 0, 3000, 300 });
    CHECK(big.w == 1000 && big.x == 0);
}

TEST(layout_screens)
{
    const std::vector<Rect> screens { { 0, 0, 1440, 900 }, { 1440, -200, 2560, 1440 } };
    CHECK(screenIndexFor({ 100, 100, 500, 500 }, screens) == 0);
    CHECK(screenIndexFor({ 1300, 100, 500, 500 }, screens) == 1); // mostly on the right
    CHECK(screenIndexFor({ 9000, 100, 50, 50 }, screens) == 1);   // off-screen -> nearest
    CHECK(screenIndexFor({}, {}) == -1);

    const Rect moved = moveToScreen({ 0, 0, 720, 900 }, screens[0], screens[1]);
    CHECK(moved == (Rect { 1440, -200, 1280, 1440 }));
    // a window wider than the target stays inside
    const Rect wide = moveToScreen({ 0, 0, 2560, 1440 }, screens[1], screens[0]);
    CHECK(wide.w <= 1440 && wide.x >= 0);
}

// ---------------------------------------------------------------- hidutil

TEST(hidutil_parse_and_build)
{
    const char* out = "(\n"
                      "        {\n"
                      "        HIDKeyboardModifierMappingDst = 30064771300;\n"
                      "        HIDKeyboardModifierMappingSrc = 30064771299;\n"
                      "    },\n"
                      "        {\n"
                      "        HIDKeyboardModifierMappingSrc = 0x700000039;\n"
                      "        HIDKeyboardModifierMappingDst = 30064771181;\n"
                      "    }\n"
                      ")\n";
    const auto list = parseHidutilMappings(out);
    CHECK(list.size() == 2);
    CHECK(list[0] == (HidMapping { 30064771299, 30064771300 }));
    CHECK(list[1] == (HidMapping { kHidCapsLock, kHidF18 }));
    CHECK(parseHidutilMappings("(null)").empty());
    CHECK(parseHidutilMappings("(\n)").empty());

    auto on = withCapsLockRemap(parseHidutilMappings("(null)"), true);
    CHECK(on.size() == 1 && on[0].dst == kHidF18);
    on = withCapsLockRemap(on, true); // idempotent
    CHECK(on.size() == 1);
    const auto off = withCapsLockRemap(list, false);
    CHECK(off.size() == 1 && off[0].src == 30064771299);

    // someone else's Caps Lock mapping survives "disable"
    const auto foreign = withCapsLockRemap({ { kHidCapsLock, 0x7000000E0 } }, false);
    CHECK(foreign.size() == 1);

    CHECK(hidutilSetJson({}) == "{\"UserKeyMapping\":[]}");
    CHECK(hidutilSetJson({ { kHidCapsLock, kHidF18 } })
          == "{\"UserKeyMapping\":[{\"HIDKeyboardModifierMappingSrc\":30064771129,"
             "\"HIDKeyboardModifierMappingDst\":30064771181}]}");
}

} // namespace

int main()
{
    for (const auto& t : registry()) {
        const int before = g_failed;
        t.fn();
        std::printf("%s %s\n", g_failed == before ? "ok  " : "FAIL", t.name);
    }
    std::printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
