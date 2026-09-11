#pragma once

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

// Physical-pixel geometry only. The caller supplies the visible dot's actual
// bounds and renders these bounds unchanged; a later minimum-size clamp would
// move odd/even pixel centers and break the join with the control's white dot.
namespace GlassAbsorbMotion {

// Product tuning, not a platform-mandated duration. The same path runs in
// opposite directions: accelerate into the dot; decelerate into the note.
// 1.25x playback speed preserves the accepted velocity distribution.
inline constexpr unsigned DurationMs = 240;
inline constexpr unsigned RestoreDurationMs = 256;

struct Frame {
    RECT rect{};
    // Time remains useful for lifecycle bookkeeping. Visual dependents must
    // use motion, so the shell, glyphs and note travel along one spatial path.
    float progress = 0.0f;
    float motion = 0.0f;
    float opacity = 1.0f;
    float dotScale = 1.0f;
};

namespace Detail {

inline double Unit(double value) {
    // NaN restarts safely; infinities behave like out-of-range timestamps.
    if (std::isnan(value)) return 0.0;
    return std::clamp(value, 0.0, 1.0);
}

inline double Smooth(double value) {
    const double t = Unit(value);
    return std::clamp(t * t * t * (t * (6.0 * t - 15.0) + 10.0), 0.0, 1.0);
}

inline double Accelerate(double time) {
    if (time <= 0.0) return 0.0;
    if (time >= 1.0) return 1.0;
    // cubic-bezier(.35, 0, .8, .2), with time on x and distance on y:
    // x(s) = 1.05s + .30s^2 - .35s^3; y(s) = .60s^2 + .40s^3.
    // dx/ds >= .6, so inversion is unique and well-conditioned. This starts
    // moving sooner than t^3, then accelerates to a bounded end speed of 4.
    // The note disappears into the dot at that end; arrival intentionally
    // doesn't decelerate into a prolonged tiny remnant.
    double lower = 0.0, upper = 1.0, s = time;
    for (unsigned iteration = 0; iteration < 32; ++iteration) {
        const double x = s * (1.05 + s * (0.30 - 0.35 * s));
        const double error = x - time;
        if (std::abs(error) < 1e-13) break;
        if (error < 0.0) lower = s;
        else upper = s;
        const double derivative = 1.05 + s * (0.60 - 1.05 * s);
        const double next = s - error / derivative;
        s = next > lower && next < upper ? next : (lower + upper) * 0.5;
    }
    return std::clamp(s * s * (0.60 + 0.40 * s), 0.0, 1.0);
}

inline LONG Coordinate(std::int64_t value) {
    return static_cast<LONG>(std::clamp(value,
        static_cast<std::int64_t>((std::numeric_limits<LONG>::min)()),
        static_cast<std::int64_t>((std::numeric_limits<LONG>::max)())));
}

inline RECT Normalize(const RECT& input, std::int64_t minimumSize = 1) {
    std::int64_t left = (std::min)(input.left, input.right);
    std::int64_t top = (std::min)(input.top, input.bottom);
    std::int64_t right = (std::max)(input.left, input.right);
    std::int64_t bottom = (std::max)(input.top, input.bottom);
    const auto maximum = static_cast<std::int64_t>((std::numeric_limits<LONG>::max)());
    if (right - left < minimumSize) {
        left = (std::min)(left, maximum - minimumSize);
        right = left + minimumSize;
    }
    if (bottom - top < minimumSize) {
        top = (std::min)(top, maximum - minimumSize);
        bottom = top + minimumSize;
    }
    return { Coordinate(left), Coordinate(top), Coordinate(right), Coordinate(bottom) };
}

inline RECT Endpoint(POINT target) {
    // Legacy point API cannot express half-pixel centers. New callers provide
    // an explicit RECT, allowing the exact diameter and parity used to draw it.
    const auto minimum = static_cast<std::int64_t>((std::numeric_limits<LONG>::min)());
    const auto maximum = static_cast<std::int64_t>((std::numeric_limits<LONG>::max)());
    const auto left = std::clamp(static_cast<std::int64_t>(target.x) - 1, minimum, maximum - 3);
    const auto top = std::clamp(static_cast<std::int64_t>(target.y) - 1, minimum, maximum - 3);
    return { Coordinate(left), Coordinate(top), Coordinate(left + 3), Coordinate(top + 3) };
}

inline RECT Interpolate(const RECT& start, const RECT& end, double progress) {
    if (progress <= 0.0) return start;
    if (progress >= 1.0) return end;
    const double startWidth = static_cast<double>(start.right) - start.left;
    const double startHeight = static_cast<double>(start.bottom) - start.top;
    const double endWidth = static_cast<double>(end.right) - end.left;
    const double endHeight = static_cast<double>(end.bottom) - end.top;
    const auto currentWidth = static_cast<std::int64_t>(std::llround(
        startWidth + (endWidth - startWidth) * progress));
    const auto currentHeight = static_cast<std::int64_t>(std::llround(
        startHeight + (endHeight - startHeight) * progress));
    const double initialX = static_cast<double>(start.left) + startWidth * 0.5;
    const double initialY = static_cast<double>(start.top) + startHeight * 0.5;
    const double targetX = static_cast<double>(end.left) + endWidth * 0.5;
    const double targetY = static_cast<double>(end.top) + endHeight * 0.5;
    const auto left = static_cast<std::int64_t>(std::llround(
        initialX + (targetX - initialX) * progress - static_cast<double>(currentWidth) * 0.5));
    const auto top = static_cast<std::int64_t>(std::llround(
        initialY + (targetY - initialY) * progress - static_cast<double>(currentHeight) * 0.5));

    // One progress value drives position and both dimensions. Round the size
    // once, then contain only the rounding remainder: no separate stretch or
    // overshoot can cross a screen edge outside the two endpoint footprints.
    const auto boundLeft = static_cast<std::int64_t>((std::min)(start.left, end.left));
    const auto boundTop = static_cast<std::int64_t>((std::min)(start.top, end.top));
    const auto boundRight = static_cast<std::int64_t>((std::max)(start.right, end.right));
    const auto boundBottom = static_cast<std::int64_t>((std::max)(start.bottom, end.bottom));
    const auto safeLeft = std::clamp(left, boundLeft, boundRight - currentWidth);
    const auto safeTop = std::clamp(top, boundTop, boundBottom - currentHeight);
    return { Coordinate(safeLeft), Coordinate(safeTop),
        Coordinate(safeLeft + currentWidth), Coordinate(safeTop + currentHeight) };
}

} // namespace Detail

inline double ShapeProgress(double normalizedTime, bool restoring) {
    const double t = Detail::Unit(normalizedTime);
    // The inverse-time entrance retraces the exact geometric path, but is
    // allowed a slightly longer duration to settle clearly into editable text.
    return restoring ? 1.0 - Detail::Accelerate(1.0 - t) : Detail::Accelerate(t);
}

inline Frame EvaluateCollapse(const RECT& full, const RECT& dot, double normalizedProgress) {
    const double t = Detail::Unit(normalizedProgress);
    const double motion = ShapeProgress(t, false);
    Frame frame;
    frame.progress = static_cast<float>(t);
    frame.motion = static_cast<float>(motion);
    frame.rect = Detail::Interpolate(Detail::Normalize(full), Detail::Normalize(dot), motion);
    // Keep the complete body visible during the pull. Crossfade only at the
    // final join; the caller's dot owns the final pixel footprint at t == 1.
    frame.opacity = motion >= 1.0 ? 0.0f :
        static_cast<float>(1.0 - Detail::Smooth((motion - 0.97) / 0.03));
    return frame;
}

inline Frame EvaluateExpand(const RECT& dot, const RECT& full, double normalizedProgress) {
    const double t = Detail::Unit(normalizedProgress);
    const double motion = ShapeProgress(t, true);
    Frame frame;
    frame.progress = static_cast<float>(t);
    frame.motion = static_cast<float>(motion);
    frame.rect = Detail::Interpolate(Detail::Normalize(dot), Detail::Normalize(full),
        motion);
    // The proxy emerges visibly from the dot, never from an empty gap. Fade
    // ownership of the white dot separately rather than fading this body out.
    frame.opacity = 1.0f;
    return frame;
}

inline Frame Evaluate(const RECT& start, POINT targetCenter, double normalizedProgress) {
    return EvaluateCollapse(Detail::Normalize(start, 3), Detail::Endpoint(targetCenter), normalizedProgress);
}

} // namespace GlassAbsorbMotion
