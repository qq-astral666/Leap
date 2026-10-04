#include "Layout.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace leap {

namespace {

// A cell of a cols×rows grid spanning `spanX` columns, with gaps between
// cells and at the screen edges.
Rect cell(const Rect& s, double gap, int cols, int rows, int col, int row, int spanX = 1, int spanY = 1)
{
    const double cw = (s.w - gap * (cols + 1)) / cols;
    const double ch = (s.h - gap * (rows + 1)) / rows;
    Rect r;
    r.x = s.x + gap + col * (cw + gap);
    r.y = s.y + gap + row * (ch + gap);
    r.w = cw * spanX + gap * (spanX - 1);
    r.h = ch * spanY + gap * (spanY - 1);
    return { std::round(r.x), std::round(r.y), std::round(r.w), std::round(r.h) };
}

double overlap(const Rect& a, const Rect& b)
{
    const double w = std::min(a.right(), b.right()) - std::max(a.x, b.x);
    const double h = std::min(a.bottom(), b.bottom()) - std::max(a.y, b.y);
    return w > 0 && h > 0 ? w * h : 0;
}

} // namespace

std::optional<Rect> layoutWindow(std::string_view c, const Rect& s, const Rect& win, double gap)
{
    gap = std::max(0.0, gap);
    if (c == "left-half") return cell(s, gap, 2, 1, 0, 0);
    if (c == "right-half") return cell(s, gap, 2, 1, 1, 0);
    if (c == "top-half") return cell(s, gap, 1, 2, 0, 0);
    if (c == "bottom-half") return cell(s, gap, 1, 2, 0, 1);
    if (c == "maximize") return cell(s, gap, 1, 1, 0, 0);
    if (c == "left-third") return cell(s, gap, 3, 1, 0, 0);
    if (c == "center-third") return cell(s, gap, 3, 1, 1, 0);
    if (c == "right-third") return cell(s, gap, 3, 1, 2, 0);
    if (c == "left-two-thirds") return cell(s, gap, 3, 1, 0, 0, 2);
    if (c == "right-two-thirds") return cell(s, gap, 3, 1, 1, 0, 2);
    if (c == "top-left") return cell(s, gap, 2, 2, 0, 0);
    if (c == "top-right") return cell(s, gap, 2, 2, 1, 0);
    if (c == "bottom-left") return cell(s, gap, 2, 2, 0, 1);
    if (c == "bottom-right") return cell(s, gap, 2, 2, 1, 1);
    if (c == "almost-maximize") {
        const double w = std::round(s.w * 0.9), h = std::round(s.h * 0.9);
        return Rect { std::round(s.x + (s.w - w) / 2), std::round(s.y + (s.h - h) / 2), w, h };
    }
    if (c == "center") {
        const double w = std::min(win.w, s.w), h = std::min(win.h, s.h);
        return Rect { std::round(s.x + (s.w - w) / 2), std::round(s.y + (s.h - h) / 2), w, h };
    }
    return std::nullopt;
}

int screenIndexFor(const Rect& window, const std::vector<Rect>& screens)
{
    int best = -1;
    double bestArea = 0;
    for (int i = 0; i < static_cast<int>(screens.size()); ++i) {
        const double a = overlap(window, screens[static_cast<size_t>(i)]);
        if (a > bestArea) {
            bestArea = a;
            best = i;
        }
    }
    if (best >= 0 || screens.empty())
        return best;
    // Off-screen: nearest center.
    double bestDist = std::numeric_limits<double>::max();
    for (int i = 0; i < static_cast<int>(screens.size()); ++i) {
        const auto& s = screens[static_cast<size_t>(i)];
        const double d = std::hypot(s.centerX() - window.centerX(), s.centerY() - window.centerY());
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

Rect moveToScreen(const Rect& win, const Rect& from, const Rect& to)
{
    const double fx = from.w > 0 ? (win.x - from.x) / from.w : 0;
    const double fy = from.h > 0 ? (win.y - from.y) / from.h : 0;
    const double fw = from.w > 0 ? win.w / from.w : 1;
    const double fh = from.h > 0 ? win.h / from.h : 1;
    Rect r { to.x + fx * to.w, to.y + fy * to.h, std::min(fw * to.w, to.w), std::min(fh * to.h, to.h) };
    r.x = std::clamp(r.x, to.x, to.right() - r.w);
    r.y = std::clamp(r.y, to.y, to.bottom() - r.h);
    return { std::round(r.x), std::round(r.y), std::round(r.w), std::round(r.h) };
}

} // namespace leap
