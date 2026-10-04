#pragma once

#include <optional>
#include <string_view>
#include <vector>

namespace leap {

// Screen rectangle, y down (the Accessibility API's coordinate system).
struct Rect {
    double x = 0, y = 0, w = 0, h = 0;

    double right() const { return x + w; }
    double bottom() const { return y + h; }
    double centerX() const { return x + w / 2; }
    double centerY() const { return y + h / 2; }
    bool operator==(const Rect&) const = default;
};

// Where a window command puts a window. `screen` is the usable area of its
// display (without menu bar and Dock), `gap` the margin between windows and
// screen edges. nullopt for unknown commands and "next-display".
std::optional<Rect> layoutWindow(std::string_view command, const Rect& screen, const Rect& window,
                                 double gap = 0);

// Display index whose area contains most of `window` (or the nearest one).
int screenIndexFor(const Rect& window, const std::vector<Rect>& screens);

// The same relative position and size on another display, kept inside it.
Rect moveToScreen(const Rect& window, const Rect& from, const Rect& to);

} // namespace leap
