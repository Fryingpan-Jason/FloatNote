#pragma once
#include <algorithm>
#include <cmath>

// Shared by the window region, editor inset, controls, and optical renderer.
class GlassGeometry {
public:
    static int MaximumRadius(float width,float height,float scale) {
        return std::max(1,int(std::floor(std::min(width,height)/(2*scale))));
    }
    static int EffectiveRadius(float requested,float width,float height,float scale) {
        const int maximum=MaximumRadius(width,height,scale);
        return std::clamp(int(std::lround(requested)),std::min(8,maximum),maximum);
    }
};
