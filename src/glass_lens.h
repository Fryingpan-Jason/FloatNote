#pragma once

// Previous monotone model, retained only as an optical comparison (M=1, bend=1).
// The primary liquid-glass route now lives in glass_bevel.h.
// Shared C++ / HLSL thin-lens model. Phi is an optical-path potential:
// source = pixel + grad(Phi), height = Phi / opticalDistance + a constant.
// Refraction and reflection therefore use the SAME surface gradient.
// Distances are physical pixels; magnification is 1..1.2, bend/dispersion 0..1.
namespace GlassLens {
#define FLOATNOTE_LENS_MATH(...) inline constexpr char Shader[] = #__VA_ARGS__; __VA_ARGS__
FLOATNOTE_LENS_MATH(
inline float lensClamp(float x,float low,float high) {
    return x<low ? low : (x>high ? high : x);
}
inline float lensPower(float magnification) {
    return 1-1/lensClamp(magnification,1,1.2f);
}
inline float lensEdgeWeight(float depth,float width) {
    if(width<=0)return 0;
    float u=1-lensClamp(depth/width,0,1);
    return u*u*u;
}
inline float lensEdgeShift(float depth,float width,float bend) {
    return .18f*lensClamp(bend,0,1)*width*lensEdgeWeight(depth,width);
}
inline float lensChannel(float dispersion,float channel) {
    return 1+.02f*lensClamp(dispersion,0,1)*lensClamp(channel,-1,1);
}
inline float lensDisplacement(float coordinate,float inward,float depth,float width,float magnification,float bend,float dispersion,float channel) {
    return (-lensPower(magnification)*coordinate+inward*lensEdgeShift(depth,width,bend))*lensChannel(dispersion,channel);
}
inline float lensPotential(float x,float y,float depth,float width,float magnification,float bend) {
    float dome=-.5f*lensPower(magnification)*(x*x+y*y);
    if(width<=0)return dome;
    float u=1-lensClamp(depth/width,0,1);
    return dome+.045f*lensClamp(bend,0,1)*width*width*(1-u*u*u*u);
}
inline float lensOpticalDistance(float scale) {
    return 24*scale;
}
)
#undef FLOATNOTE_LENS_MATH

// The paraboloid gives exactly the requested green-channel central zoom.
// The quartic edge shoulder adds curvature without changing that central zoom.
// Its shift is strongest AT the silhouette: images need not line up across a
// refractive boundary. Shift and its first two derivatives vanish only at the
// INNER shoulder, joining the dome smoothly instead of a flat identity region.
// For width <= radius, every RGB Jacobian has radial eigenvalue >=
// 1 - 1.02*(1-1/1.2 + 3*.18) = .2792; tangential >= .6464.
// The fixed optical distance converts optical-path slope to geometric slope;
// it is a visual thin-lens approximation, not a full Snell/caustic simulation.
}
