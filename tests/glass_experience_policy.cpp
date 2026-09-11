#include "../experiments/glass_experience_policy.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

void Require(bool valid, const char* message) {
    if (!valid) throw std::runtime_error(message);
}

int main() { try {
    using namespace GlassExperiencePolicy;
    const std::array<COLORREF, 5> original{RGB(10,20,30), RGB(40,50,60), RGB(70,80,90),
        RGB(100,110,120), RGB(130,140,150)};
    const COLORREF custom = RGB(12,34,56);
    auto colors = PromoteColor(original, custom);
    Require(colors[0] == custom, "custom color must be immediately visible first");
    Require(std::equal(colors.begin()+1, colors.end(), original.begin()),
        "new color must preserve older order and evict the final swatch");
    colors = PromoteColor(colors, original[2]);
    const std::array<COLORREF, 5> promoted{original[2], custom, original[0], original[1], original[3]};
    Require(colors == promoted, "existing color must move to front without losing another swatch");
    Require(PromoteColor(colors, colors[0]) == colors, "choosing first swatch must be idempotent");
    Require(PromoteColor(original, original.back())[1] == original.front(), "last swatch promotion must retain first");
    for (unsigned i=0; i<24; ++i) {
        const COLORREF next = RGB(170+i, i, 220-i);
        colors = PromoteColor(colors, next);
        Require(colors[0] == next, "overflow must retain latest selected color");
        for (std::size_t j=0; j<colors.size(); ++j)
            Require(std::count(colors.begin(), colors.end(), colors[j]) == 1,
                "repeated insertion must not duplicate swatches");
    }
    Require(PromoteColor(std::array<COLORREF,1>{custom}, original[0])[0] == original[0],
        "one-slot palette must replace its previous color");
    Require(PromoteColor(original, 0xab000000 | custom)[0] == custom, "selection must contain RGB only");

    std::size_t thresholds=0;
    for (UINT dpi : {96u, 120u, 137u, 144u, 168u, 192u, 213u, 240u, 288u}) {
        const int limit=MulDiv(FoldHeightDip, static_cast<int>(dpi), 96);
        Require(ShouldFold(limit, dpi), "fold height must include exact rounded physical threshold");
        Require(ShouldFold(limit-1, dpi), "one pixel below threshold must fold");
        Require(!ShouldFold(limit+1, dpi), "one pixel above threshold must stay expanded");
        Require(ShouldFold(MulDiv(MinimumHeightDip, static_cast<int>(dpi), 96), dpi),
            "minimum note height must fold");
        ++thresholds;
    }
    Require(ShouldFold(28, 0) && !ShouldFold(29, 0), "unknown DPI must use 100 percent");
    std::cout << "PASS palette insertion, promotion, overflow, RGB mask; " << thresholds
        << " DPI fold boundaries\n";
    return 0;
} catch(const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; } }
