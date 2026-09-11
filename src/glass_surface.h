#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// Optional historical comparison. Baked only when the old-height-field optical
// candidate is selected; the new local cross-section needs no height texture.
// A single screened-Poisson cap over the rounded-rectangle domain:
//     h - lambda^2 * Laplacian(h) = 1, h(boundary) = 0.
// The interior tends to a flat plateau. Unlike nearest-edge distance normals,
// this field has no diagonal switch when the optical band exceeds the radius.
// Baked only on geometry changes; no particles or per-frame CPU simulation.
class GlassSurface {
public:
    static int MaximumRadius(float width,float height,float scale) {
        return std::max(1,int(std::floor(std::min(width,height)/(2*scale))));
    }
    static int EffectiveRadius(float requested,float width,float height,float scale) {
        const int maximum=MaximumRadius(width,height,scale);
        return std::clamp(int(std::lround(requested)),std::min(8,maximum),maximum);
    }
    struct Texel {float h,dx,dy,dxy;}; // derivatives in grid-coordinate units
    int columns=0,rows=0,iterations=0;
    float stepX=1,stepY=1,lambda=1,lastDelta=0;
    unsigned builds=0;
    std::vector<Texel> texels;
private:
    float cachedWidth=0,cachedHeight=0,cachedRadius=0,cachedBand=0,cachedSpacing=0;
public:
    bool Build(float width,float height,float radius,float band,float spacing) {
        radius=std::clamp(radius,1.0f,std::min(width,height)*.5f);
        if(width==cachedWidth && height==cachedHeight && radius==cachedRadius &&
           band==cachedBand && spacing==cachedSpacing && !texels.empty())return false;
        cachedWidth=width;cachedHeight=height;cachedRadius=radius;cachedBand=band;cachedSpacing=spacing;
        const float hw=width*.5f,hh=height*.5f;
        const int nx=std::clamp(int(std::ceil(hw/spacing))+1,9,129);
        const int ny=std::clamp(int(std::ceil(hh/spacing))+1,9,129);
        columns=2*nx-1;rows=2*ny-1;stepX=hw/(nx-1);stepY=hh/(ny-1);
        lambda=std::max(1.0f,band/3.0f);
        struct Node {float l=0,r=0,t=0,b=0,inverse=0;};
        std::vector<Node> nodes(nx*ny);
        std::vector<float> h(nx*ny,0);
        auto boundaryX=[&](float y){float d=std::max(0.0f,y-(hh-radius));return hw-radius+std::sqrt(std::max(0.0f,radius*radius-d*d));};
        auto boundaryY=[&](float x){float d=std::max(0.0f,x-(hw-radius));return hh-radius+std::sqrt(std::max(0.0f,radius*radius-d*d));};
        auto inside=[&](int x,int y){return x<nx && y<ny && x*stepX<boundaryX(y*stepY)-.0001f && y*stepY<hh-.0001f;};
        for(int y=0;y<ny;y++)for(int x=0;x<nx;x++) {
            if(!inside(x,y))continue;
            const float px=x*stepX,py=y*stepY;
            const float dl=stepX,dt=stepY;
            // Subcell Dirichlet boundaries reduce stair steps at rounded corners.
            const float dr=inside(x+1,y)?stepX:std::max(stepX*.02f,boundaryX(py)-px);
            const float db=inside(x,y+1)?stepY:std::max(stepY*.02f,boundaryY(px)-py);
            Node& n=nodes[y*nx+x];const float k=2*lambda*lambda;
            n.l=k/(dl*(dl+dr));n.r=k/(dr*(dl+dr));n.t=k/(dt*(dt+db));n.b=k/(db*(dt+db));
            n.inverse=1/(1+n.l+n.r+n.t+n.b);
            const float qx=px-(hw-radius),qy=py-(hh-radius);
            const float depth=radius-std::hypot(std::max(qx,0.0f),std::max(qy,0.0f))-std::min(std::max(qx,qy),0.0f);
            h[y*nx+x]=1-std::exp(-depth/lambda);
        }
        // Reflecting boundaries at the two symmetry axes; red-black SOR.
        for(iterations=0;iterations<320;iterations++) {
            lastDelta=0;
            for(int parity=0;parity<2;parity++)for(int y=0;y<ny-1;y++)for(int x=(parity+y)%2;x<nx-1;x+=2) {
                const int i=y*nx+x;const Node& n=nodes[i];if(n.inverse==0)continue;
                const float target=(1+n.l*h[y*nx+(x?x-1:1)]+n.r*h[i+1]+
                    n.t*h[(y?y-1:1)*nx+x]+n.b*h[i+nx])*n.inverse;
                const float next=std::clamp(h[i]+1.55f*(target-h[i]),0.0f,1.0f);
                lastDelta=std::max(lastDelta,std::abs(next-h[i]));h[i]=next;
            }
            if(lastDelta<.000001f){++iterations;break;}
        }
        texels.assign(columns*rows,{});
        for(int y=0;y<rows;y++)for(int x=0;x<columns;x++)
            texels[y*columns+x].h=h[std::abs(y-(ny-1))*nx+std::abs(x-(nx-1))];
        auto at=[&](int x,int y)->Texel& {return texels[std::clamp(y,0,rows-1)*columns+std::clamp(x,0,columns-1)];};
        for(int y=0;y<rows;y++)for(int x=0;x<columns;x++) {
            auto& v=at(x,y);
            v.dx=x==0?(-3*v.h+4*at(1,y).h-at(2,y).h)*.5f:
                x==columns-1?(3*v.h-4*at(x-1,y).h+at(x-2,y).h)*.5f:(at(x+1,y).h-at(x-1,y).h)*.5f;
            v.dy=y==0?(-3*v.h+4*at(x,1).h-at(x,2).h)*.5f:
                y==rows-1?(3*v.h-4*at(x,y-1).h+at(x,y-2).h)*.5f:(at(x,y+1).h-at(x,y-1).h)*.5f;
        }
        for(int y=0;y<rows;y++)for(int x=0;x<columns;x++)
            at(x,y).dxy=(at(x,y+1).dx-at(x,y-1).dx)/(y==0 || y==rows-1?1.0f:2.0f);
        ++builds;return true;
    }

    // CPU counterpart of the shader's bicubic Hermite interpolation, useful
    // for checking value/gradient continuity without opening any windows.
    struct Sample {float h,dx,dy;};
    Sample Evaluate(float x,float y) const {
        const float gx=std::clamp(x/stepX,0.0f,float(columns-1)),gy=std::clamp(y/stepY,0.0f,float(rows-1));
        const int ix=std::min(int(gx),columns-2),iy=std::min(int(gy),rows-2);
        const float u=gx-ix,v=gy-iy;
        auto basis=[](float t,float* b,float* db){
            b[0]=2*t*t*t-3*t*t+1;b[1]=-2*t*t*t+3*t*t;b[2]=t*t*t-2*t*t+t;b[3]=t*t*t-t*t;
            db[0]=6*t*t-6*t;db[1]=-db[0];db[2]=3*t*t-4*t+1;db[3]=3*t*t-2*t;
        };
        float bx[4],by[4],dx[4],dy[4];basis(u,bx,dx);basis(v,by,dy);
        const auto& a=texels[iy*columns+ix];const auto& b=texels[iy*columns+ix+1];
        const auto& c=texels[(iy+1)*columns+ix];const auto& d=texels[(iy+1)*columns+ix+1];
        const float m[4][4]={{a.h,b.h,a.dx,b.dx},{c.h,d.h,c.dx,d.dx},{a.dy,b.dy,a.dxy,b.dxy},{c.dy,d.dy,c.dxy,d.dxy}};
        Sample s{};for(int j=0;j<4;j++)for(int i=0;i<4;i++){
            s.h+=by[j]*m[j][i]*bx[i];s.dx+=by[j]*m[j][i]*dx[i]/stepX;s.dy+=dy[j]*m[j][i]*bx[i]/stepY;
        }
        return s;
    }
};
