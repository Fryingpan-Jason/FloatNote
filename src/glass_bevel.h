#pragma once
#include <cmath>

// Height profiles and layered rim treatment adapted from liquid-dom,
// Copyright (c) 2026 Andras Prifer, MIT. See THIRD_PARTY_NOTICES.md.
// Source: dd342ab663d718e8b4edf4cd39983b9a873360a7/packages/core/src/shaders.ts
// C++/HLSL share these scalar functions so numerical checks exercise shipped math.
namespace GlassBevel {
#define FLOATNOTE_BEVEL_MATH(...) inline constexpr char Shader[] = #__VA_ARGS__; __VA_ARGS__
FLOATNOTE_BEVEL_MATH(
struct BevelProfile {float height;float derivative;};
inline float bevelClamp(float x,float a,float b){return x<a?a:(x>b?b:x);}
inline BevelProfile bevelProfile(float progress,float profile) {
    float t=bevelClamp(progress,0,1),u=1-t;
    float inside=profile>.5f && profile<1.5f ? 1-u*u : 1-u*u*u*u;
    float root=sqrt(inside>.0001f?inside:.0001f);
    BevelProfile result;result.height=root;
    result.derivative=profile>.5f && profile<1.5f ? u/root : 2*u*u*u/root;
    if(profile>1.5f){
        // Equivalent to the upstream blend, evaluated symmetrically to avoid
        // float cancellation where the concave lip joins its flat interior.
        float blend=t*t*t*(t*(t*6-15)+10);
        float inverse=u*u*u*(u*(u*6-15)+10);
        if(t>.5f)blend=1-inverse;else inverse=1-blend;
        float db=30*t*t*u*u;
        float concave=inside>.0001f ? u*u*u*u/(1+root) : 1-root;
        result.derivative=result.derivative*(inverse-blend)+(concave-root)*db;
        result.height=inverse*root+blend*concave;
    }
    return result;
}
inline float bevelSlope(float depth,float width,float relief,float profile) {
    if(width<=0 || depth>=width)return 0;
    return bevelClamp(bevelProfile(depth/width,profile).derivative*relief/width,-11.43005f,11.43005f);
}
inline float bevelHeight(float depth,float width,float base,float relief,float profile) {
    float t=width>0 ? depth/width : 1;
    return base+relief*bevelProfile(t,profile).height;
}
inline float bevelTravel(float slope,float height,float ior) {
    float nz=1/sqrt(1+slope*slope),nx=slope*nz,eta=1/ior;
    float k=1-eta*eta*(1-nz*nz);
    float term=eta*nz-sqrt(k>0?k:0);
    float rayX=term*nx,rayZ=-eta+term*nz;
    return rayX*height/(-rayZ>.0001f?-rayZ:.0001f);
}
)
#undef FLOATNOTE_BEVEL_MATH
}
