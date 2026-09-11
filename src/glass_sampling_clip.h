#pragma once
#include <windows.h>
#include <algorithm>

// HRGN is a binary scissor, not the visible glass contour. Leave two physical
// pixels for the shader's subpixel reconstruction and clip to the client bounds.
// Pixels outside the visible contour are filled with the unfiltered captured
// scene by the shader, so the enlarged scissor cannot expose an Acrylic fringe.
inline HRGN CreateGlassSamplingClip(int width,int height,int radius) {
    constexpr int guard=2;
    radius=std::clamp(radius,0,std::min(width,height)/2);
    HRGN region=CreateRoundRectRgn(-guard,-guard,width+guard+1,height+guard+1,
                                  (radius+guard)*2,(radius+guard)*2);
    HRGN bounds=CreateRectRgn(0,0,width,height);
    if(!region || !bounds){if(region)DeleteObject(region);if(bounds)DeleteObject(bounds);return nullptr;}
    const int result=CombineRgn(region,region,bounds,RGN_AND);
    DeleteObject(bounds);
    if(result==ERROR){DeleteObject(region);return nullptr;}
    return region;
}
