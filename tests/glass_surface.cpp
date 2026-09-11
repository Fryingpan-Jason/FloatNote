#include "../src/glass_surface.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
    try {
        const auto started=std::chrono::steady_clock::now();
        Require(GlassSurface::MaximumRadius(548,264,2)==66,"maximum radius must follow short side and DPI");
        Require(GlassSurface::EffectiveRadius(500,548,264,2)==66,"oversized radius not clamped");
        Require(GlassSurface::EffectiveRadius(48,548,264,2)==48,"default radius changed unexpectedly");
        std::cout<<"PASS maximum radius / DPI clamping\n";
        for(auto geometry:std::vector<std::vector<float>>{{864,680,56,78,4},{864,680,16,96,4},{864,680,96,16,4},{360,260,28,39,2},{3000,2200,32,96,4},{548,264,132,24,4},{600,600,300,58,4}}){
            GlassSurface field;float w=geometry[0],h=geometry[1],r=geometry[2],band=geometry[3];
            Require(field.Build(w,h,r,band,geometry[4]),"initial bake missing");
            Require(!field.Build(w,h,r,band,geometry[4]),"identical geometry should reuse field");
            Require(field.lastDelta<.00005f,"solver did not converge sufficiently");
            const auto center=field.Evaluate(w*.5f,h*.5f);
            Require(std::abs(center.dx)<1e-6 && std::abs(center.dy)<1e-6,"center symmetry broken");
            float maximumJump=0,maximumDerivativeError=0,maximumSymmetryError=0;
            auto compare=[&](float x,float y){
                auto a=field.Evaluate(x,y),mirror=field.Evaluate(w-x,y);
                Require(std::isfinite(a.h+a.dx+a.dy),"non-finite surface");
                maximumSymmetryError=std::max({maximumSymmetryError,std::abs(a.h-mirror.h),std::abs(a.dx+mirror.dx),std::abs(a.dy-mirror.dy)});
                const float epsilon=.05f;
                float gx=(field.Evaluate(x+epsilon,y).h-field.Evaluate(x-epsilon,y).h)/(2*epsilon);
                float gy=(field.Evaluate(x,y+epsilon).h-field.Evaluate(x,y-epsilon).h)/(2*epsilon);
                maximumDerivativeError=std::max({maximumDerivativeError,std::abs(gx-a.dx),std::abs(gy-a.dy)});
            };
            // Cross cell boundaries; test actual derivatives of interpolated
            // heights, not agreement with a separately interpolated normal map.
            for(int i=2;i<field.columns-2;i++){
                float x=i*field.stepX,y=r+band*.31f;
                auto a=field.Evaluate(x-.001f,y),b=field.Evaluate(x+.001f,y);
                maximumJump=std::max({maximumJump,std::abs(a.dx-b.dx),std::abs(a.dy-b.dy)});compare(x,y);
            }
            for(int i=2;i<field.rows-2;i++){
                float x=r+band*.31f,y=i*field.stepY;
                auto a=field.Evaluate(x,y-.001f),b=field.Evaluate(x,y+.001f);
                maximumJump=std::max({maximumJump,std::abs(a.dx-b.dx),std::abs(a.dy-b.dy)});compare(x,y);
            }
            // The old nearest-edge implementation jumps across these diagonals
            // when band > radius. Sweep all four corners, not just one preset.
            for(float s=1;s<band;s+=1)for(int cx:{0,1})for(int cy:{0,1}){
                float x=r+s,y=r+s; if(cx)x=w-x;if(cy)y=h-y;
                auto a=field.Evaluate(x-.001f,y+.001f),b=field.Evaluate(x+.001f,y-.001f);
                maximumJump=std::max({maximumJump,std::abs(a.dx-b.dx),std::abs(a.dy-b.dy)});compare(x,y);
            }
            Require(maximumJump<.001f,"gradient discontinuity");
            Require(maximumDerivativeError<.0005f,"normal is not the height gradient");
            Require(maximumSymmetryError<.0001f,"mirror asymmetry");
            std::cout<<"PASS "<<w<<"x"<<h<<" r="<<r<<" band="<<band<<" grid="<<field.columns<<"x"<<field.rows
                <<" iterations="<<field.iterations<<" delta="<<field.lastDelta<<" gradientJump="<<maximumJump
                <<" derivativeError="<<maximumDerivativeError<<" symmetryError="<<maximumSymmetryError<<"\n";
        }
        std::cout<<"Numerical checks completed in "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()<<" ms\n";
    }catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
