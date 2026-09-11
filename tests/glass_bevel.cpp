#include "../src/glass_bevel.h"
#include "../src/glass_geometry.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
    try {
        float maximumError=0,maximumTravel=0;unsigned samples=0,reversedSegments=0;
        for(float profile:{0.0f,1.0f,2.0f}) {
            for(int i=1;i<1000;i++) {
                const float t=i*.001f,e=.0001f;const auto value=GlassBevel::bevelProfile(t,profile);
                const float derivative=(GlassBevel::bevelProfile(t+e,profile).height-GlassBevel::bevelProfile(t-e,profile).height)/(2*e);
                const float error=std::abs(derivative-value.derivative)/(1+std::abs(value.derivative));
                maximumError=std::max(maximumError,error);
                Require(std::isfinite(value.height+value.derivative) && value.height>=-.00001f && value.height<=1.00001f,"invalid height profile");
                if(error>=.004f)std::cerr<<"profile="<<profile<<" t="<<t<<" analytic="<<value.derivative<<" finiteDifference="<<derivative<<" error="<<error<<"\n";
                Require(error<.004f,"profile derivative does not match its height");
            }
            for(float width:{0.0f,1.0f,8.0f,24.0f,48.0f})for(float base:{0.0f,8.0f,32.0f})
            for(float relief:{0.0f,24.0f,32.0f})for(float ior:{1.48f,1.5f,1.52f}) {
                float previousSource=0;
                for(int i=0;i<=1000;i++) {
                    const float depth=width*i/1000;
                    const float slope=GlassBevel::bevelSlope(depth,width,relief,profile);
                    const float height=GlassBevel::bevelHeight(depth,width,base,relief,profile);
                    const float travel=GlassBevel::bevelTravel(slope,height,ior);
                    // Independent angle form of Snell's law, not another call
                    // to the shader's algebraic vector expression.
                    const float angle=std::atan(slope);
                    const float expected=height*std::tan(std::asin(std::sin(angle)/ior)-angle);
                    Require(std::isfinite(travel) && std::abs(travel-expected)<.0002f,"ray does not match Snell angle construction");
                    Require(std::abs(travel)<=base+relief+.001f,"ray escaped supported source halo");
                    if(depth>=width)Require(travel==0,"flat interior was distorted");
                    if(relief==0)Require(travel==0,"zero relief should leave a flat surface");
                    maximumTravel=std::max(maximumTravel,std::abs(travel));
                    const float source=depth-travel;
                    if(i>0 && source<previousSource)++reversedSegments;
                    previousSource=source;++samples;
                    for(float scale:{1.0f,1.5f,2.0f}) {
                        const float s=GlassBevel::bevelSlope(depth*scale,width*scale,relief*scale,profile);
                        const float h=GlassBevel::bevelHeight(depth*scale,width*scale,base*scale,relief*scale,profile);
                        const float scaled=GlassBevel::bevelTravel(s,h,ior)/scale;
                        Require(std::abs(scaled-travel)<.002f,"DPI changed optical shape");
                    }
                }
            }
        }
        Require(GlassGeometry::EffectiveRadius(80,588,314,2)==78,"effective corner radius mismatch");
        Require(GlassBevel::bevelSlope(0,24,24,0)>0 && GlassBevel::bevelSlope(12,24,24,2)<0,"profile families lost opposite slopes");
        // Continuity means source values meet at the inner join. Circular and
        // quartic sections need not have identical higher-order smoothness.
        for(float p:{0.0f,1.0f,2.0f}){
            float s=GlassBevel::bevelSlope(23.9999f,24,24,p);
            float h=GlassBevel::bevelHeight(23.9999f,24,8,24,p);
            Require(std::abs(GlassBevel::bevelTravel(s,h,1.5f))<.001f,"inner join has a displacement jump");
        }
        std::cout<<"PASS "<<samples<<" profile/ray samples; derivative relative error="<<maximumError
            <<", max travel="<<maximumTravel<<" DIP; flat interior, zero relief, DPI and bounded source\n";
        std::cout<<"Observed "<<reversedSegments<<" local reversed segments (diagnostic; neither required nor rejected).\n";
    }catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
