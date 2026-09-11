#include "../experiments/glass_absorb_motion.h"

#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {

void Check(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
}

bool Equal(const RECT& a, const RECT& b) {
    return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
}

double Width(const RECT& r) { return static_cast<double>(r.right) - r.left; }
double Height(const RECT& r) { return static_cast<double>(r.bottom) - r.top; }
double CenterX(const RECT& r) { return static_cast<double>(r.left) + Width(r) * 0.5; }
double CenterY(const RECT& r) { return static_cast<double>(r.top) + Height(r) * 0.5; }

using Evaluator = GlassAbsorbMotion::Frame (*)(const RECT&, const RECT&, double);

void SweepDirection(const RECT& start, const RECT& end, Evaluator evaluate, bool collapse) {
    const auto first = evaluate(start, end, 0);
    const auto last = evaluate(start, end, 1);
    Check(Equal(first.rect, start), "exact valid start rectangle including pixel parity");
    Check(Equal(last.rect, end), "exact final rectangle including pixel parity");
    Check(first.motion == 0 && last.motion == 1, "spatial progress endpoint contract");
    Check(first.opacity == 1 && last.opacity == (collapse ? 0 : 1), "visible opacity endpoints");
    Check(Equal(evaluate(start, end, -1).rect, start), "negative time clamps to start");
    Check(Equal(evaluate(start, end, 5).rect, end), "late time clamps to end");
    Check(Equal(evaluate(start, end, std::numeric_limits<double>::quiet_NaN()).rect, start), "NaN safe start");
    Check(Equal(evaluate(start, end, -std::numeric_limits<double>::infinity()).rect, start), "negative infinity start");
    Check(Equal(evaluate(start, end, std::numeric_limits<double>::infinity()).rect, end), "positive infinity end");
    auto previous = first;
    const double dx = CenterX(end) - CenterX(start);
    const double dy = CenterY(end) - CenterY(start);
    for (int step = 1; step <= 1000; ++step) {
        const double t = static_cast<double>(step) / 1000;
        const auto frame = evaluate(start, end, t);
        Check(Width(frame.rect) >= 1 && Height(frame.rect) >= 1, "positive pixel dimensions without caller minimum clamp");
        Check(Width(end) >= Width(start) ? Width(frame.rect) >= Width(previous.rect) :
            Width(frame.rect) <= Width(previous.rect), "width moves monotonically toward endpoint");
        Check(Height(end) >= Height(start) ? Height(frame.rect) >= Height(previous.rect) :
            Height(frame.rect) <= Height(previous.rect), "height moves monotonically toward endpoint");
        Check(frame.rect.left >= (std::min)(start.left, end.left) &&
            frame.rect.top >= (std::min)(start.top, end.top) &&
            frame.rect.right <= (std::max)(start.right, end.right) &&
            frame.rect.bottom <= (std::max)(start.bottom, end.bottom), "no movement beyond endpoint footprints");
        const double x = CenterX(frame.rect), y = CenterY(frame.rect);
        // Integer sizes alternate odd/even; centering may shift by half a pixel.
        Check(dx >= 0 ? x >= CenterX(previous.rect) - 0.5 : x <= CenterX(previous.rect) + 0.5,
            "horizontal center progression");
        Check(dy >= 0 ? y >= CenterY(previous.rect) - 0.5 : y <= CenterY(previous.rect) + 0.5,
            "vertical center progression");
        Check(frame.progress >= previous.progress && std::isfinite(frame.progress), "finite monotonic progress");
        Check(std::abs(frame.progress - t) < 1e-7, "lifecycle progress remains linear time");
        Check(std::isfinite(frame.motion) && frame.motion >= previous.motion && frame.motion <= 1,
            "finite monotonic spatial progress without overshoot");
        Check(std::abs(frame.motion - GlassAbsorbMotion::ShapeProgress(t, !collapse)) < 1e-7,
            "dependent visuals receive the note geometry's spatial progress");
        Check(std::isfinite(frame.opacity) && frame.opacity >= 0 && frame.opacity <= previous.opacity,
            "finite non-increasing opacity");
        Check(!collapse || frame.motion > 0.97 || frame.opacity == 1,
            "collapse keeps its body until the last three percent of spatial travel");
        Check(frame.dotScale == 1, "endpoint dot does not pulse away from matching bounds");
        previous = frame;
    }
}

void SweepPair(const RECT& full, const RECT& dot) {
    using namespace GlassAbsorbMotion;
    SweepDirection(full, dot, EvaluateCollapse, true);
    SweepDirection(dot, full, EvaluateExpand, false);
    for (int step = 0; step <= 100; ++step) {
        const double t = static_cast<double>(step) / 100;
        const RECT a = EvaluateCollapse(full, dot, t).rect;
        const RECT b = EvaluateExpand(dot, full, 1 - t).rect;
        Check(std::abs(static_cast<double>(a.left) - b.left) <= 1 &&
            std::abs(static_cast<double>(a.top) - b.top) <= 1 &&
            std::abs(static_cast<double>(a.right) - b.right) <= 1 &&
            std::abs(static_cast<double>(a.bottom) - b.bottom) <= 1, "restoration follows the same path backwards");
    }
}

void CheckCurve() {
    using GlassAbsorbMotion::ShapeProgress;
    // Independent points obtained by substituting s=.25/.5/.75 in the
    // selected Bezier, rather than reproducing the inverse-time evaluator.
    Check(std::abs(ShapeProgress(0.27578125, false) - 0.04375) < 1e-10, "Bezier quarter-parameter reference");
    Check(std::abs(ShapeProgress(0.55625, false) - 0.20) < 1e-10, "Bezier half-parameter reference");
    Check(std::abs(ShapeProgress(0.80859375, false) - 0.50625) < 1e-10, "Bezier three-quarter-parameter reference");
    for (bool restoring : {false, true}) {
        Check(ShapeProgress(0, restoring) == 0 && ShapeProgress(1, restoring) == 1,
            "curve exact endpoints");
        Check(ShapeProgress(-1, restoring) == 0 && ShapeProgress(2, restoring) == 1,
            "curve finite out-of-range safety");
        Check(ShapeProgress(std::numeric_limits<double>::quiet_NaN(), restoring) == 0 &&
            ShapeProgress(-std::numeric_limits<double>::infinity(), restoring) == 0 &&
            ShapeProgress(std::numeric_limits<double>::infinity(), restoring) == 1,
            "curve non-finite input safety");
    }
    for (int step = 0; step <= 1000; ++step) {
        const double t = static_cast<double>(step) / 1000;
        Check(std::abs(ShapeProgress(t, false) + ShapeProgress(1 - t, true) - 1) < 1e-12,
            "entrance is the strict time reverse of the exit");
    }
    Check(ShapeProgress(0.25, false) > 0.025 && ShapeProgress(0.25, false) < 0.05,
        "exit starts visibly without a long dead interval or initial jump");
    const double h = 1e-5;
    const double endSpeed = (1 - ShapeProgress(1 - h, false)) / h;
    const double startSpeed = ShapeProgress(h, true) / h;
    Check(endSpeed > 3.99 && endSpeed < 4.01 && std::abs(startSpeed - endSpeed) < 1e-8,
        "crisp exit and entrance have matched bounded fourfold peak speed");
    double previousSpeed = 0;
    for (int step = 1; step <= 100; ++step) {
        const double t = static_cast<double>(step) / 100;
        const double speed = (ShapeProgress(t, false) - ShapeProgress(t - 0.01, false)) / 0.01;
        Check(speed > previousSpeed && speed <= 4.01, "exit speed rises throughout without a late brake or spike");
        previousSpeed = speed;
    }
    // At 95% clock time the body is still far enough away that clock-based
    // fading would erase the join. At 99.9% it hands over to the dot.
    const RECT full{0, 0, 1000, 80}, dot{495, -20, 505, -10};
    const auto approach = GlassAbsorbMotion::EvaluateCollapse(full, dot, 0.95);
    const auto join = GlassAbsorbMotion::EvaluateCollapse(full, dot, 0.999);
    Check(approach.motion < 0.97 && approach.opacity == 1, "body retained during fast final approach");
    Check(join.motion > 0.97 && join.opacity > 0 && join.opacity < 1, "body crossfades only at spatial join");
}

} // namespace

int main() {
    using namespace GlassAbsorbMotion;
    static_assert(DurationMs == 240 && RestoreDurationMs == 256);
    CheckCurve();
    unsigned pairs = 0;
    for (const int dpi : {96, 108, 120, 126, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 264, 288}) {
        const LONG w = 181 * dpi / 96, h = 29 * dpi / 96;
        for (const POINT origin : {POINT{0, 0}, POINT{-1920, -1080}, POINT{3242, 1004}}) {
            const RECT full{origin.x, origin.y, origin.x + w, origin.y + h};
            for (const POINT target : {POINT{origin.x + w / 2, origin.y - 8},
                POINT{origin.x - 10, origin.y - 8}, POINT{origin.x + w + 10, origin.y + h + 8}}) {
                for (const LONG diameter : {LONG{1}, LONG{5}, LONG{8}, LONG{11}}) {
                    // Deliberately vary odd/even parity: the RECT is definitive,
                    // not a POINT followed by a separately clamped diameter.
                    const RECT dot{target.x, target.y, target.x + diameter, target.y + diameter};
                    SweepPair(full, dot); ++pairs;
                }
            }
        }
    }
    SweepPair({-1000000000, -30, 1000000000, 30}, {1000000000, -80, 1000000009, -71}); ++pairs;
    SweepPair({-2147483647, -2147483647, 2147483647, 2147483647},
        {2147483639, -2147483647, 2147483647, -2147483639}); ++pairs;
    SweepPair({-1, 5, 4, 6}, {-7, -4, -4, -2}); ++pairs;

    const RECT normal{20, 40, 620, 96};
    const RECT dot{316, 24, 324, 32};
    for (const RECT malformed : {RECT{10, 10, 10, 10}, RECT{200, 90, 10, 20},
        RECT{2147483647, 2147483647, 2147483647, 2147483647}}) {
        const auto a = EvaluateCollapse(malformed, dot, 0.4);
        const auto b = EvaluateExpand(malformed, normal, 0.4);
        Check(Width(a.rect) >= 1 && Height(a.rect) >= 1 && Width(b.rect) >= 1 && Height(b.rect) >= 1,
            "malformed rectangle normalized safely");
    }
    // Large specimen avoids rounding noise when comparing equal time intervals.
    const RECT curveFull{0, 0, 100000, 100000}, curveDot{49996, -8, 50004, 0};
    double collapseWidth = Width(curveFull), previousContraction = 0;
    double expandWidth = Width(curveDot), previousExpansion = Width(curveFull);
    for (int step = 1; step <= 10; ++step) {
        const double t = static_cast<double>(step) / 10;
        const double cw = Width(EvaluateCollapse(curveFull, curveDot, t).rect);
        const double ew = Width(EvaluateExpand(curveDot, curveFull, t).rect);
        Check(collapseWidth - cw > previousContraction, "collapse accelerates over equal time intervals");
        Check(ew - expandWidth < previousExpansion, "expansion slows over equal time intervals");
        previousContraction = collapseWidth - cw; collapseWidth = cw;
        previousExpansion = ew - expandWidth; expandWidth = ew;
    }
    const auto legacy = Evaluate(normal, POINT{320, 32}, 1);
    Check(Width(legacy.rect) == 3 && Height(legacy.rect) == 3 && legacy.opacity == 0,
        "legacy point API keeps its 3px endpoint contract");
    std::printf("PASS: %u paired geometry sweeps across 16 DPIs; exact dot bounds, reverse path, curves, fade timing, and invalid-input safety.\n", pairs);
}
