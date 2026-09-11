#include <windows.h>
#include "../src/glass_editor_layout.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

void Require(bool valid,const char* message) { if(!valid)throw std::runtime_error(message); }
float Distance(float x,float y,int width,int height,float radius) {
    const float qx=std::abs(x-width*.5f)-(width*.5f-radius);
    const float qy=std::abs(y-height*.5f)-(height*.5f-radius);
    return std::hypot(std::max(qx,0.0f),std::max(qy,0.0f))+std::min(std::max(qx,qy),0.0f)-radius;
}
int main() {try {
    size_t cases=0;
    for(float scale:{1.0f,1.25f,1.5f,2.0f,2.5f,3.0f})
        for(auto size:{std::array<int,2>{72,72},{72,140},{180,90},{227,90},{320,160},
                      {400,260},{300,300},{1024,1024},
                      {72,24},{76,24},{72,28},{76,28},{72,32},{76,32},
                      {72,40},{76,40},{72,44},{76,44},{72,48},{76,48},
                      {180,24},{320,28},{227,48}})
            for(float request:{8.0f,45.0f,48.0f,4096.0f}) {
                const int width=int(std::lround(size[0]*scale)),height=int(std::lround(size[1]*scale));
                const float radius=std::min(request*scale,std::min(width,height)*.5f);
                const auto layout=GlassEditorLayout::Calculate(width,height,request,scale);
                for(const auto box:{layout.editor,layout.grip}) {
                    Require(box.Width()>0 && box.Height()>0,"nonpositive input geometry");
                    Require(box.left>=0 && box.top>=0 && box.right<=width && box.bottom<=height,"input escapes window");
                    for(auto point:{std::array<int,2>{box.left,box.top},{box.right,box.top},
                                   {box.left,box.bottom},{box.right,box.bottom}})
                        if(Distance(float(point[0]),float(point[1]),width,height,radius)>.01f) {
                            std::cerr<<"geometry "<<width<<'x'<<height<<" scale "<<scale<<" radius "<<radius
                                <<" corner "<<point[0]<<','<<point[1]<<'\n';
                            throw std::runtime_error("input leaves rounded contour");
                        }
                }
                Require(layout.editor.right<layout.grip.left,"grip overlaps editor hit area");
                if(size[1]<=48) {
                    Require(layout.grip.Height()==int(std::lround(18*scale)),"shallow resize grip must keep its full hit height");
                    Require(std::abs(layout.grip.top-(height-layout.grip.Height())/2)<=1,
                        "shallow resize grip must stay vertically centered");
                }
                if(radius/scale>96) {
                    const int oldInset=int(std::ceil(radius*.292894f))+int(std::lround(3*scale));
                    Require(layout.editor.left-oldInset<=int(std::ceil(8*scale)),"large circle lost excessive text width");
                }
                ++cases;
            }
    const auto shallow=GlassEditorLayout::Calculate(453,180,45,2);
    Require(shallow.editor.top==24 && shallow.editor.bottom==156,"shallow note should have 12 DIP vertical padding");
    Require(shallow.editor.Height()==132,"shallow note should reclaim full text height");
    const auto live=GlassEditorLayout::Calculate(453,180,36,2.5f);
    Require(live.editor.top==30 && live.editor.bottom==150,"250 percent note should have 12 DIP vertical padding");
    Require(live.editor.Height()==120,"250 percent note should reclaim full text height");

    // A hidden native EDIT verifies the independent formatting rectangle and
    // preservation of text/selection/undo. No live note or visible GUI is used.
    auto nativeEditCheck=[](const GlassEditorLayout::Layout& layout,int dpi,int oldWidth,int oldHeight,int minimumLines) {
    HWND host=CreateWindowExW(0,L"STATIC",L"",WS_POPUP,0,0,453,180,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    HWND edit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN,
        0,0,oldWidth,oldHeight,host,nullptr,GetModuleHandleW(nullptr),nullptr);
    Require(host && edit,"hidden EDIT creation failed");
    HFONT font=CreateFontW(-MulDiv(11,dpi,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
    Require(font!=nullptr,"fixture font creation failed");
    SendMessageW(edit,WM_SETFONT,reinterpret_cast<WPARAM>(font),FALSE);
    SendMessageW(edit,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,0);
    RECT oldFormat{};SendMessageW(edit,EM_GETRECT,0,reinterpret_cast<LPARAM>(&oldFormat));
    HDC dc=GetDC(edit);const auto previous=SelectObject(dc,font);TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);
    SelectObject(dc,previous);ReleaseDC(edit,dc);
    const std::wstring text=L"First line\r\nSecond line\r\nThird line";
    SendMessageW(edit,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(text.c_str()));
    SendMessageW(edit,EM_SETSEL,2,8);
    Require(SendMessageW(edit,EM_CANUNDO,0,0)!=0,"fixture must create undo record");
    SetWindowPos(edit,nullptr,0,0,1,1,SWP_NOZORDER|SWP_NOACTIVATE);
    SetWindowPos(edit,nullptr,layout.editor.left,layout.editor.top,layout.editor.Width(),layout.editor.Height(),SWP_NOZORDER|SWP_NOACTIVATE);
    RECT format{};GetClientRect(edit,&format);SendMessageW(edit,EM_SETRECTNP,0,reinterpret_cast<LPARAM>(&format));
    RECT actual{};SendMessageW(edit,EM_GETRECT,0,reinterpret_cast<LPARAM>(&actual));
    Require(actual.bottom-actual.top>=minimumLines*metrics.tmHeight,"new EDIT cannot fit expected complete 11 pt lines");
    wchar_t value[128]{};GetWindowTextW(edit,value,128);Require(value==text,"resize changed note text");
    DWORD start=0,end=0;SendMessageW(edit,EM_GETSEL,reinterpret_cast<WPARAM>(&start),reinterpret_cast<LPARAM>(&end));
    Require(start==2 && end==8,"resize changed selection");
    Require(SendMessageW(edit,EM_CANUNDO,0,0)!=0,"resize discarded undo");
    SendMessageW(edit,EM_UNDO,0,0);Require(GetWindowTextLengthW(edit)==0,"original undo operation no longer works");
    DestroyWindow(host);DeleteObject(font);
    std::cout<<"PASS native EDIT DPI "<<dpi<<": editor "<<oldWidth<<"x"<<oldHeight<<" -> "
        <<layout.editor.Width()<<"x"<<layout.editor.Height()<<" px; formatting height "<<oldFormat.bottom-oldFormat.top
        <<" -> "<<actual.bottom-actual.top<<", font line height "<<metrics.tmHeight<<" px; at least "<<minimumLines
        <<" complete lines; text/selection/undo preserved\n";
    };
    nativeEditCheck(shallow,192,385,86,3);
    // Exact live note: 453x180 px at 250%, with the requested 45 DIP corner
    // clamped to 36 DIP by its 72 DIP height. Old padding was 14 + 30 DIP.
    nativeEditCheck(live,240,453-2*MulDiv(14,240,96),180-MulDiv(14,240,96)-MulDiv(30,240,96),2);
    std::cout<<"PASS "<<cases<<" rounded layouts\n";
    return 0;
} catch(const std::exception& error) {std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
