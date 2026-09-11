#pragma once
#include <windows.h>
#include <array>
#include <cstddef>

namespace GlassExperiencePolicy {
inline constexpr int MinimumHeightDip = 24;
inline constexpr int FoldHeightDip = 28;

inline bool ShouldFold(int physicalHeight, UINT dpi) {
    return physicalHeight <= MulDiv(FoldHeightDip, static_cast<int>(dpi ? dpi : 96), 96);
}

// The swatch palette starts with distinct RGB values. Choosing an existing
// value moves it to the front; choosing a new value evicts only the oldest.
// This preserves that uniqueness without introducing an empty visible swatch.
template<std::size_t N>
std::array<COLORREF, N> PromoteColor(std::array<COLORREF, N> colors, COLORREF selected) {
    static_assert(N > 0, "A visible color palette must have at least one swatch");
    selected &= 0x00ffffff;
    std::array<COLORREF, N> result{};
    result[0] = selected;
    std::size_t next = 1;
    for (COLORREF color : colors) {
        if (color != selected && next < N) result[next++] = color;
    }
    return result;
}
}
