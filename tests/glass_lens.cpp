#include "../src/glass_lens.h"
#include "../src/glass_geometry.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Point {float x,y;};
struct Lens {float width,height,radius,band,zoom,bend,dispersion;};
struct Location {float vx,vy,depth,nx,ny;};
Location Locate(Point p,const Lens& lens) {
    const float vx=p.x-lens.width*.5f,vy=p.y-lens.height*.5f;
    const float qx=std::abs(vx)-(lens.width*.5f-lens.radius),qy=std::abs(vy)-(lens.height*.5f-lens.radius);
    const float cx=std::max(qx,0.0f),cy=std::max(qy,0.0f),length=std::hypot(cx,cy);
    const float depth=lens.radius-length-std::min(std::max(qx,qy),0.0f);
    const float nx=length>1e-5f?cx/length:(qx>qy?1.0f:0.0f);
    const float ny=length>1e-5f?cy/length:(qx>qy?0.0f:1.0f);
    return {vx,vy,depth,-std::copysign(nx,vx),-std::copysign(ny,vy)};
}
Point Map(Point p,const Lens& lens,float channel) {
    const auto at=Locate(p,lens);const float band=std::min(lens.band,lens.radius);
    return {p.x+GlassLens::lensDisplacement(at.vx,at.nx,at.depth,band,lens.zoom,lens.bend,lens.dispersion,channel),
        p.y+GlassLens::lensDisplacement(at.vy,at.ny,at.depth,band,lens.zoom,lens.bend,lens.dispersion,channel)};
}
float Potential(Point p,const Lens& lens) {
    const auto at=Locate(p,lens);
    return GlassLens::lensPotential(at.vx,at.vy,at.depth,std::min(lens.band,lens.radius),lens.zoom,lens.bend);
}
int main(){
    try {
        Require(GlassGeometry::MaximumRadius(548,264,2)==66,"radius must follow short side and DPI");
        Require(GlassGeometry::EffectiveRadius(500,548,264,2)==66,"oversized radius not clamped");
        Require(GlassGeometry::EffectiveRadius(48,548,264,2)==48,"default radius changed");
        // All integer geometry settings and channel extremes. Outer refraction
        // is allowed to offset the background; only the INNER join is flat.
        float minimum=10,maximum=0;
        for(int zoom=100;zoom<=120;zoom++)for(int bend=0;bend<=100;bend++)
        for(float dispersion:{0.0f,1.0f})for(float channel:{-1.0f,0.0f,1.0f}) {
            auto source=[&](float t){return t+GlassLens::lensDisplacement(t-2,1,t,1,zoom*.01f,bend*.01f,dispersion,channel);};
            float previous=source(0);
            for(int i=1;i<=500;i++){
                const float t=i/500.0f,value=source(t),jacobian=(value-previous)*500;
                Require(jacobian>.277f && jacobian<1.002f,"sampling reverses or compresses the source");
                minimum=std::min(minimum,jacobian);maximum=std::max(maximum,jacobian);previous=value;
            }
        }
        Require(GlassLens::lensEdgeShift(0,16,1)>0,"outer boundary incorrectly pinned to identity");
        Require(GlassLens::lensEdgeShift(16,16,1)==0 && GlassLens::lensEdgeShift(17,16,1)==0,"edge leaked into central zoom");
        Require(GlassLens::lensEdgeShift(15.999f,16,1)<1e-9f,"inner shoulder not smooth");
        Require(GlassLens::lensEdgeShift(0,0,1)==0,"zero width must disable only the edge");
        std::cout<<"PASS slider sweep: RGB radial Jacobian "<<minimum<<".."<<maximum<<"; outer refraction and C2 inner join\n";

        // UI percentage must mean actual central enlargement, independent of
        // both edge controls; test at three DPI scales.
        for(float zoom:{1.0f,1.04f,1.08f,1.2f})for(float bend:{0.0f,.45f,1.0f})for(float width:{0.0f,8.0f,48.0f}) {
            Lens lens{864,680,96,width,zoom,bend,0};
            Point p{470,370};const auto mapped=Map(p,lens,0);
            Require(std::abs(mapped.x-(432+(p.x-432)/zoom))<.0001f &&
                std::abs(mapped.y-(340+(p.y-340)/zoom))<.0001f,"edge controls changed central magnification");
            for(float scale:{1.0f,1.5f,2.0f}) {
                Lens scaled{lens.width*scale,lens.height*scale,lens.radius*scale,width*scale,zoom,bend,0};
                const Point nearEdge{780,40};const auto a=Map(nearEdge,lens,0),b=Map({nearEdge.x*scale,nearEdge.y*scale},scaled,0);
                Require(std::hypot(a.x-b.x/scale,a.y-b.y/scale)<.001f,"DPI changed the lens shape");
            }
        }
        std::cout<<"PASS exact central zoom / edge independence / DPI scaling\n";

        float smallestEigenvalue=10,largestEigenvalue=0,smallestDeterminant=10,gradientError=0;
        unsigned checked=0;
        for(auto geometry:std::vector<std::vector<float>>{{588,314,90},{468,284,142},{864,680,16},{600,600,300}})
        for(auto preset:std::vector<std::vector<float>>{{1.04f,.45f,16},{1,1,48},{1.2f,1,48},{1.2f,1,8},{1.04f,0,0}})
        for(float scale:{1.0f,1.5f,2.0f}) {
            Lens lens{geometry[0]*scale,geometry[1]*scale,geometry[2]*scale,preset[2]*scale,preset[0],preset[1],1};
            const float band=std::min(lens.band,lens.radius);
            auto check=[&](Point p){
                for(float channel:{-1.0f,0.0f,1.0f}){
                    const float e=.08f*scale;
                    const auto l=Map({p.x-e,p.y},lens,channel),r=Map({p.x+e,p.y},lens,channel);
                    const auto t=Map({p.x,p.y-e},lens,channel),b=Map({p.x,p.y+e},lens,channel);
                    const float xx=(r.x-l.x)/(2*e),yx=(r.y-l.y)/(2*e),xy=(b.x-t.x)/(2*e),yy=(b.y-t.y)/(2*e);
                    const float off=(xy+yx)*.5f,trace=(xx+yy)*.5f,delta=std::hypot((xx-yy)*.5f,off);
                    const float lower=trace-delta,upper=trace+delta,det=xx*yy-xy*yx;
                    Require(std::isfinite(det) && lower>.275f && upper<1.005f && det>.17f,"corner mapping folds or jumps");
                    smallestEigenvalue=std::min(smallestEigenvalue,lower);largestEigenvalue=std::max(largestEigenvalue,upper);
                    smallestDeterminant=std::min(smallestDeterminant,det);++checked;
                }
                // Independently differentiate the optical potential to catch
                // unrelated reflection normals and refraction displacements.
                const auto at=Locate(p,lens);const float e=.1f*scale;
                if(at.depth>e*2 && at.depth<band-e*2) {
                    const float dx=(Potential({p.x+e,p.y},lens)-Potential({p.x-e,p.y},lens))/(2*e);
                    const float dy=(Potential({p.x,p.y+e},lens)-Potential({p.x,p.y-e},lens))/(2*e);
                    const auto source=Map(p,lens,0);const float opticalDistance=GlassLens::lensOpticalDistance(scale);
                    const float error=std::hypot(dx-(source.x-p.x),dy-(source.y-p.y))/opticalDistance;
                    gradientError=std::max(gradientError,error);
                    Require(error<.015f,"reflection slope disagrees with refractive surface gradient");
                }
            };
            for(int sideX:{0,1})for(int sideY:{0,1})for(int angle=0;angle<=90;angle+=5)
            for(int step=0;step<=40;step++) {
                const float a=angle*3.14159265358979323846f/180,depth=band*step/40;
                Point p{lens.width-lens.radius+(lens.radius-depth)*std::cos(a),lens.radius-(lens.radius-depth)*std::sin(a)};
                if(sideX)p.x=lens.width-p.x;if(sideY)p.y=lens.height-p.y;
                check(p);
            }
            for(int i=0;i<=80;i++) {
                check({lens.width*.5f,band*i/80});check({band*i/80,lens.height*.5f});
            }
        }
        std::cout<<"PASS "<<checked<<" 2D RGB probes: Jacobian eigenvalues "<<smallestEigenvalue<<".."<<largestEigenvalue
            <<", determinant >= "<<smallestDeterminant<<", surface gradient error <= "<<gradientError<<"\n";
    }catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
