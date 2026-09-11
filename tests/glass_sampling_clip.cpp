#include "../src/glass_sampling_clip.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <array>

// Independently exercise the real GDI scissor against the continuous shader
// contour's complete 4x4 reconstruction footprint, including odd dimensions.
float Distance(float x,float y,int w,int h,int radius) {
    float qx=std::abs(x-w*.5f)-(w*.5f-radius),qy=std::abs(y-h*.5f)-(h*.5f-radius);
    return std::hypot(std::max(qx,0.0f),std::max(qy,0.0f))+std::min(std::max(qx,qy),0.0f)-radius;
}
int main(){try{
    size_t supportPixels=0,previouslyCut=0;
    for(auto size:{std::array<int,3>{360,180,90},{361,181,90},{720,360,180},{1000,640,96},
                   {180,90,45},{201,203,64},{596,340,134},{321,181,8}}) {
        auto [w,h,r]=size;HRGN region=CreateGlassSamplingClip(w,h,r);
        HRGN old=CreateRoundRectRgn(0,0,w+1,h+1,r*2,r*2);
        if(!region || !old)throw std::runtime_error("could not create region");
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){
            const float d=Distance(x+.5f,y+.5f,w,h,r);
            if(d>1.1f || d< -2)continue;
            float coverage=0;
            for(int sy=0;sy<4;sy++)for(int sx=0;sx<4;sx++)
                coverage+=std::clamp(.5f-Distance(x+(sx+.5f)/4,y+(sy+.5f)/4,w,h,r),0.0f,1.0f);
            if(coverage>.00001f){
                ++supportPixels;
                if(!PtInRegion(old,x,y))++previouslyCut;
                if(!PtInRegion(region,x,y))throw std::runtime_error("new clip cuts a covered shader pixel");
            }
        }
        RECT bounds{};GetRgnBox(region,&bounds);
        if(bounds.left!=0 || bounds.top!=0 || bounds.right!=w || bounds.bottom!=h)
            throw std::runtime_error("scissor escaped client bounds");
        DeleteObject(region);DeleteObject(old);
    }
    if(previouslyCut==0)throw std::runtime_error("fixture did not reproduce the previous clipping defect");
    std::cout<<"PASS "<<supportPixels<<" edge footprint pixels in 8 geometries; old clip discarded "
             <<previouslyCut<<", new clip discarded 0; client bounds unchanged.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
