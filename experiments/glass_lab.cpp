#define FLOATNOTE_GLASS_LAB 1
#include "../src/main.cpp"

namespace {
HWND labControls=nullptr,labMode=nullptr,labBlur=nullptr,labTint=nullptr,labBase=nullptr,labStatus=nullptr;
HWND labCorner=nullptr,labRelief=nullptr,labHighlight=nullptr;
HWND labDispersion=nullptr,labAdaptive=nullptr,labWidth=nullptr;
HWND labModel=nullptr,labProfile=nullptr,labReflection=nullptr,labOpticalOnly=nullptr;
HWND labRoughness=nullptr,labEnvironmentTint=nullptr;
HFONT labFont=nullptr;
bool labScreenshot=false,labResumeAfterScreenshot=false;
std::wstring labNotice;
void UpdateLabValueLabels();
void RefreshGlassLabGeometry() {
    if(!labCorner || !labWidth)return;
    const int maximum=NoteMaximumCornerRadius(),effective=NoteCornerRadius();
    if(SendMessageW(labCorner,TBM_GETRANGEMAX,0,0)!=maximum)
        SendMessageW(labCorner,TBM_SETRANGE,TRUE,MAKELPARAM(std::min(8,maximum),maximum));
    if(SendMessageW(labCorner,TBM_GETPOS,0,0)!=effective)SendMessageW(labCorner,TBM_SETPOS,TRUE,effective);
    const int bandMaximum=std::min(48,effective),band=std::min(bandMaximum,int(std::lround(g_backdrop.edgeWidth)));
    if(SendMessageW(labWidth,TBM_GETRANGEMAX,0,0)!=bandMaximum)SendMessageW(labWidth,TBM_SETRANGE,TRUE,MAKELPARAM(0,bandMaximum));
    if(SendMessageW(labWidth,TBM_GETPOS,0,0)!=band)SendMessageW(labWidth,TBM_SETPOS,TRUE,band);
    UpdateLabValueLabels();
}

void LoadGlassLabPreferences() {
    const auto path=g_dataDirectory/L"material.ini";
    g_backdrop.mode=std::clamp(int(GetPrivateProfileIntW(L"Material",L"mode",2,path.c_str())),-1,2);
    g_backdrop.blur=std::clamp(int(GetPrivateProfileIntW(L"Material",L"blurQuarter",4,path.c_str())),0,80)*.25f;
    g_backdrop.cornerRadius=float(std::clamp(int(GetPrivateProfileIntW(L"Material",L"cornerRadius",48,path.c_str())),8,4096));
    // The new optical units cannot be reconstructed from old bend/zoom values.
    // Preserve note/size/blur, but start old configs at the documented research preset.
    if(GetPrivateProfileIntW(L"Material",L"opticsVersion",0,path.c_str())<4)return;
    auto value=[&](const wchar_t* key,int fallback,int low,int high){return std::clamp(int(GetPrivateProfileIntW(L"Material",key,fallback,path.c_str())),low,high);};
    g_backdrop.opticalModel=value(L"opticalModel",0,0,2);g_backdrop.edgeProfile=value(L"edgeProfile",0,0,2);
    g_backdrop.baseThickness=float(value(L"baseThicknessDip",32,0,32));
    g_backdrop.reliefHeight=float(value(L"reliefHeightDip",10,0,32));
    g_backdrop.edgeWidth=float(value(L"edgeWidthDip",30,0,48));
    g_backdrop.reflection=value(L"reflectionPercent",50,0,100)*.01f;
    g_backdrop.highlight=value(L"highlightPercent",80,0,100)*.01f;
    g_backdrop.edgeRoughness=value(L"edgeRoughnessPercent",30,0,100)*.01f;
    g_backdrop.environmentTint=value(L"environmentTintPercent",35,0,100)*.01f;
    // The old highlight's 100% had a much lower peak. Migrate appearance only;
    // preserve the user's optical shape, note, blur, colour and geometry.
    if(value(L"finishVersion",0,0,1)==0){g_backdrop.highlight=.8f;g_backdrop.reflection=.5f;}
    g_backdrop.dispersion=value(L"dispersionPercent",0,0,100)*.01f;
    g_backdrop.adaptiveContrast=value(L"adaptiveContrast",1,0,1)!=0;
    g_backdrop.opticalOnly=value(L"opticalOnly",0,0,1)!=0;
}
void SaveGlassLabPreferences() {
    AtomicWrite(g_dataDirectory/L"material.ini","[Material]\r\nopticsVersion=4\r\nfinishVersion=1\r\nmode="+std::to_string(g_backdrop.mode)+
        "\r\nblurQuarter="+std::to_string(int(std::lround(g_backdrop.blur*4)))+
        "\r\nopticalModel="+std::to_string(g_backdrop.opticalModel)+"\r\nedgeProfile="+std::to_string(g_backdrop.edgeProfile)+
        "\r\nbaseThicknessDip="+std::to_string(int(std::lround(g_backdrop.baseThickness)))+
        "\r\nreliefHeightDip="+std::to_string(int(std::lround(g_backdrop.reliefHeight)))+
        "\r\ncornerRadius="+std::to_string(int(std::lround(g_backdrop.cornerRadius)))+
        "\r\nedgeWidthDip="+std::to_string(int(std::lround(g_backdrop.edgeWidth)))+
        "\r\nreflectionPercent="+std::to_string(int(std::lround(g_backdrop.reflection*100)))+
        "\r\nedgeRoughnessPercent="+std::to_string(int(std::lround(g_backdrop.edgeRoughness*100)))+
        "\r\nenvironmentTintPercent="+std::to_string(int(std::lround(g_backdrop.environmentTint*100)))+
        "\r\nhighlightPercent="+std::to_string(int(std::lround(g_backdrop.highlight*100)))+
        "\r\ndispersionPercent="+std::to_string(int(std::lround(g_backdrop.dispersion*100)))+
        "\r\nadaptiveContrast="+std::to_string(g_backdrop.adaptiveContrast)+"\r\nopticalOnly="+std::to_string(g_backdrop.opticalOnly)+"\r\n");
}
void UpdateLabValueLabels() {
    auto label=[](int id,const wchar_t* name,float value,const wchar_t* unit) {
        std::wostringstream text; text<<name<<L"："<<value<<unit;
        SetWindowTextW(GetDlgItem(labControls,id+1000),text.str().c_str());
    };
    label(802,L"模糊强度",g_backdrop.blur,L" px");
    label(803,L"背景遮色",float(g_settings.opacityPercent),L"%");
    label(804,L"基础光程",g_backdrop.baseThickness,L" 逻辑像素");
    const auto corner=L"圆角大小："+std::to_wstring(NoteCornerRadius())+L" / "+std::to_wstring(NoteMaximumCornerRadius());
    SetWindowTextW(GetDlgItem(labControls,1810),corner.c_str());
    label(811,L"起伏高度",g_backdrop.reliefHeight,L" 逻辑像素");
    label(816,L"边缘宽度",std::min(g_backdrop.edgeWidth,float(NoteCornerRadius())),L" 逻辑像素");
    label(823,L"背景反光",float(std::lround(g_backdrop.reflection*100)),L"%");
    label(812,L"细高光",float(std::lround(g_backdrop.highlight*100)),L"%");
    label(824,L"边缘柔化",float(std::lround(g_backdrop.edgeRoughness*100)),L"%");
    label(825,L"反光染色",float(std::lround(g_backdrop.environmentTint*100)),L"%");
    label(813,L"色散强度",float(std::lround(g_backdrop.dispersion*100)),L"%");
}
void LabStatus() {
    if(labTint && SendMessageW(labTint,TBM_GETPOS,0,0)!=g_settings.opacityPercent) {
        SendMessageW(labTint,TBM_SETPOS,TRUE,g_settings.opacityPercent);
        UpdateLabValueLabels();
    }
    const auto status=g_backdrop.mode>=0 && !g_glassActive ? std::wstring(g_glassStatus) : g_backdrop.Status();
    if(labStatus) SetWindowTextW(labStatus,((labNotice.empty()?L"":labNotice+L"\r\n")+status).c_str());
#ifndef FLOATNOTE_LOCAL_DESKTOP
    const auto value=g_backdrop.Diagnostics();
    const int length=WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),nullptr,0,nullptr,nullptr);
    std::string encoded(length,'\0');
    WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),encoded.data(),length,nullptr,nullptr);
    std::ofstream(ExecutableDirectory()/L"lab-status.txt",std::ios::binary)<<encoded;
#endif
}
void RefreshLabControls() {
    if(!labControls) return;
    SendMessageW(labMode,CB_SETCURSEL,g_backdrop.mode+1,0);
    SendMessageW(labBlur,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.blur*4)));
    SendMessageW(labBase,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.baseThickness)));
    RefreshGlassLabGeometry();
    SendMessageW(labRelief,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.reliefHeight)));
    SendMessageW(labModel,CB_SETCURSEL,g_backdrop.opticalModel,0);
    SendMessageW(labProfile,CB_SETCURSEL,g_backdrop.edgeProfile,0);
    SendMessageW(labReflection,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.reflection*100)));
    SendMessageW(labRoughness,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.edgeRoughness*100)));
    SendMessageW(labEnvironmentTint,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.environmentTint*100)));
    SendMessageW(labOpticalOnly,BM_SETCHECK,g_backdrop.opticalOnly?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(labHighlight,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.highlight*100)));
    SendMessageW(labDispersion,TBM_SETPOS,TRUE,int(std::lround(g_backdrop.dispersion*100)));
    SendMessageW(labAdaptive,BM_SETCHECK,g_backdrop.adaptiveContrast?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(labTint,TBM_SETPOS,TRUE,g_settings.opacityPercent);
    EnableWindow(labBlur,g_backdrop.mode>0); EnableWindow(labBase,g_backdrop.mode==2 && g_backdrop.opticalModel!=2);
    EnableWindow(labRelief,g_backdrop.mode==2 && g_backdrop.opticalModel!=2);
    EnableWindow(labModel,g_backdrop.mode==2);EnableWindow(labProfile,g_backdrop.mode==2 && g_backdrop.opticalModel==0);
    EnableWindow(labOpticalOnly,g_backdrop.mode==2);
    EnableWindow(labHighlight,g_backdrop.mode==2 && !g_backdrop.opticalOnly);
    EnableWindow(labReflection,g_backdrop.mode==2 && !g_backdrop.opticalOnly);
    EnableWindow(labRoughness,g_backdrop.mode==2 && !g_backdrop.opticalOnly);
    EnableWindow(labEnvironmentTint,g_backdrop.mode==2 && !g_backdrop.opticalOnly);
    EnableWindow(labDispersion,g_backdrop.mode==2);EnableWindow(labAdaptive,g_backdrop.mode==2 && !g_backdrop.opticalOnly);
    EnableWindow(labWidth,g_backdrop.mode==2);
    EnableWindow(labMode,!labScreenshot);EnableWindow(GetDlgItem(labControls,807),!labScreenshot);
    EnableWindow(GetDlgItem(labControls,805),g_backdrop.mode>0 && !labScreenshot);
    SetWindowTextW(GetDlgItem(labControls,814),labScreenshot?L"结束截图（恢复先前状态）":L"准备并截图");
    SetWindowTextW(GetDlgItem(labControls,805),g_backdrop.Frozen() ? L"恢复实时背景" : L"冻结背景（便于对照）");
    UpdateLabValueLabels(); LabStatus();
}
void EndLabScreenshot(bool restore) {
    KillTimer(labControls,3);
    const bool resume=restore && labScreenshot && labResumeAfterScreenshot;
    labScreenshot=false;labResumeAfterScreenshot=false;labNotice.clear();
    if(resume)g_backdrop.Freeze(false);
    RefreshLabControls();
}
void BeginLabScreenshot() {
    if(labScreenshot){EndLabScreenshot(true);return;}
    const bool resume=g_backdrop.mode>0 && !g_backdrop.Frozen();
    if(!g_backdrop.PrepareScreenshot(g_window)){
        labNotice=L"背景尚未就绪，请稍后重试。";LabStatus();return;
    }
    labScreenshot=true;labResumeAfterScreenshot=resume;
    labNotice=L"截图已准备，完成后点「结束截图」。";
    RefreshLabControls();
    // Yield for the visible, capture-enabled frame and labels to be presented.
    // No timeout auto-resume: it could re-exclude the note during a slow snip.
    SetTimer(labControls,3,180,nullptr);
}
void SelectLabMaterial(int mode) {
    if(labScreenshot)EndLabScreenshot(false);
    g_backdrop.Freeze(false); g_backdrop.mode=std::clamp(mode,-1,2);
    g_backdrop.Changed(); ApplyVisuals(g_window); UpdateMenuLabels();
    RefreshLabControls(); SaveGlassLabPreferences();
}
LRESULT CALLBACK LabProcedure(HWND window,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_HSCROLL:
        if(reinterpret_cast<HWND>(lp)==labBlur) g_backdrop.blur=float(SendMessageW(labBlur,TBM_GETPOS,0,0))*.25f;
        if(reinterpret_cast<HWND>(lp)==labBase) g_backdrop.baseThickness=float(SendMessageW(labBase,TBM_GETPOS,0,0));
        if(reinterpret_cast<HWND>(lp)==labTint) SetOpacityPercent(int(SendMessageW(labTint,TBM_GETPOS,0,0)),false);
        if(reinterpret_cast<HWND>(lp)==labCorner) {
            g_backdrop.cornerRadius=float(SendMessageW(labCorner,TBM_GETPOS,0,0)); UpdateLayout(g_window);
        }
        if(reinterpret_cast<HWND>(lp)==labRelief) g_backdrop.reliefHeight=float(SendMessageW(labRelief,TBM_GETPOS,0,0));
        if(reinterpret_cast<HWND>(lp)==labReflection) g_backdrop.reflection=float(SendMessageW(labReflection,TBM_GETPOS,0,0))*.01f;
        if(reinterpret_cast<HWND>(lp)==labRoughness) g_backdrop.edgeRoughness=float(SendMessageW(labRoughness,TBM_GETPOS,0,0))*.01f;
        if(reinterpret_cast<HWND>(lp)==labEnvironmentTint) g_backdrop.environmentTint=float(SendMessageW(labEnvironmentTint,TBM_GETPOS,0,0))*.01f;
        if(reinterpret_cast<HWND>(lp)==labWidth) g_backdrop.edgeWidth=float(SendMessageW(labWidth,TBM_GETPOS,0,0));
        if(reinterpret_cast<HWND>(lp)==labHighlight) g_backdrop.highlight=float(SendMessageW(labHighlight,TBM_GETPOS,0,0))*.01f;
        if(reinterpret_cast<HWND>(lp)==labDispersion) g_backdrop.dispersion=float(SendMessageW(labDispersion,TBM_GETPOS,0,0))*.01f;
        g_backdrop.Changed(); SetTimer(window,2,300,nullptr); UpdateLabValueLabels(); LabStatus(); return 0;
    case WM_COMMAND:
        if(LOWORD(wp)==820 && HIWORD(wp)==CBN_SELCHANGE){g_backdrop.opticalModel=int(SendMessageW(labModel,CB_GETCURSEL,0,0));
            g_backdrop.Changed();RefreshLabControls();SaveGlassLabPreferences();}
        if(LOWORD(wp)==821 && HIWORD(wp)==CBN_SELCHANGE){g_backdrop.edgeProfile=int(SendMessageW(labProfile,CB_GETCURSEL,0,0));
            g_backdrop.Changed();RefreshLabControls();SaveGlassLabPreferences();}
        if(LOWORD(wp)==822){g_backdrop.opticalOnly=SendMessageW(labOpticalOnly,BM_GETCHECK,0,0)==BST_CHECKED;
            g_backdrop.Changed();RefreshLabControls();SaveGlassLabPreferences();}
        if(LOWORD(wp)==801 && HIWORD(wp)==CBN_SELCHANGE) SelectLabMaterial(int(SendMessageW(labMode,CB_GETCURSEL,0,0))-1);
        if(LOWORD(wp)==805) {g_backdrop.Freeze(!g_backdrop.Frozen()); RefreshLabControls();}
        if(LOWORD(wp)==814) BeginLabScreenshot();
        if(LOWORD(wp)==815){g_backdrop.adaptiveContrast=SendMessageW(labAdaptive,BM_GETCHECK,0,0)==BST_CHECKED;
            g_backdrop.Changed();SaveGlassLabPreferences();RefreshLabControls();}
        if(LOWORD(wp)==806) {ShowWindow(window,SW_HIDE);SyncExperienceUI();}
        if(LOWORD(wp)==807) {
            g_backdrop.opticalModel=0;g_backdrop.edgeProfile=0;g_backdrop.edgeWidth=30;
            g_backdrop.baseThickness=32;g_backdrop.reliefHeight=10;g_backdrop.blur=1;
            g_backdrop.reflection=.5f;g_backdrop.highlight=.8f;g_backdrop.dispersion=0;
            g_backdrop.edgeRoughness=.3f;g_backdrop.environmentTint=.35f;SetOpacityPercent(0);
            g_backdrop.adaptiveContrast=true;g_backdrop.opticalOnly=false;
            if(g_backdrop.mode!=2)SelectLabMaterial(2);
            else{g_backdrop.Changed();RefreshLabControls();SaveGlassLabPreferences();}
        }
        if(LOWORD(wp)==808) {EnterEditor(); SetWindowPos(g_window,nullptr,1150,380,1000,640,SWP_NOACTIVATE|SWP_NOZORDER);ApplyWindowStacking();}
        return 0;
    case WM_TIMER:
        if(wp==2) {KillTimer(window,2); SaveGlassLabPreferences();}
        else if(wp==3){
            KillTimer(window,3);
            if(labScreenshot){
                // Bare URI is the legacy unpackaged-app launch path. The new
                // callback protocol requires MSIX identity; do not register one.
                const auto result=reinterpret_cast<INT_PTR>(ShellExecuteW(window,L"open",L"ms-screenclip:",nullptr,nullptr,SW_SHOWNORMAL));
                labNotice=result>32 ? L"完成后点「结束截图」。\r\n未弹出时可按 Win+Shift+S。" : L"已准备好，请按 Win+Shift+S。\r\n完成后点「结束截图」。";
                LabStatus();
            }
        }
        else LabStatus();
        return 0;
    case WM_CLOSE: SaveGlassLabPreferences();ShowWindow(window,SW_HIDE);SyncExperienceUI();return 0;
    case WM_DESTROY: SaveGlassLabPreferences(); if(labFont) {DeleteObject(labFont);labFont=nullptr;} labControls=nullptr; return 0;
    }
    return DefWindowProcW(window,msg,wp,lp);
}
void OpenGlassLabControls() {
    if(labControls && IsWindow(labControls)) {ShowWindow(labControls,SW_SHOWNOACTIVATE);RefreshLabControls();return;}
    WNDCLASSW cls{};cls.lpfnWndProc=LabProcedure;cls.hInstance=g_instance;
    cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    cls.lpszClassName=L"FloatNote.GlassLab.Controls";RegisterClassW(&cls);
    const float scale=std::max(1.0f,float(GetDpiForWindow(g_window))/96.0f);
    auto px=[&](int value){return int(std::lround(value*scale));};
    labFont=CreateFontW(-px(13),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
    labControls=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"FloatNote 材质实验 · 光线与曲面",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,100,100,px(560),px(820),g_window,nullptr,g_instance,nullptr);
    auto control=[&](const wchar_t* type,const wchar_t* text,DWORD style,int id,int x,int y,int w,int h){
        HWND child=CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(w),px(h),labControls,
            reinterpret_cast<HMENU>(INT_PTR(id)),g_instance,nullptr);
        SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(labFont),TRUE);return child;
    };
    control(L"STATIC",L"先用推荐预设；形变与边缘外观可分别调整。",0,0,18,16,524,22);
    labMode=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,801,18,46,524,180);
    for(const wchar_t* label:{L"普通透明（无模糊）",L"系统毛玻璃（对照）",L"轻模糊（无折射）",L"液态玻璃"})
        SendMessageW(labMode,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));
    control(L"STATIC",L"光学模型",0,0,18,86,80,22);
    labModel=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,820,102,82,440,160);
    for(int i=0;i<3;i++)SendMessageW(labModel,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(GlassLabBackdrop::ModelName(i)));
    control(L"STATIC",L"边缘截面",0,0,18,122,80,22);
    labProfile=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,821,102,118,440,160);
    for(const wchar_t* label:{L"四次截面（外陡内平）",L"圆弧截面",L"凸凹组合截面"})
        SendMessageW(labProfile,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));
    auto slider=[&](const wchar_t* label,int id,int x,int y,int minimum,int maximum,int value){
        control(L"STATIC",label,0,id+1000,x,y,250,20);
        HWND s=control(TRACKBAR_CLASSW,label,TBS_HORZ|WS_TABSTOP,id,x-4,y+20,258,30);
        SendMessageW(s,TBM_SETRANGE,TRUE,MAKELPARAM(minimum,maximum));SendMessageW(s,TBM_SETPOS,TRUE,value);return s;
    };
    control(L"STATIC",L"形状与背景",0,0,18,153,250,22);
    control(L"STATIC",L"光线与边缘",0,0,290,153,250,22);
    labBlur=slider(L"模糊强度",802,18,180,0,80,4);
    labTint=slider(L"背景遮色",803,18,233,0,100,g_settings.opacityPercent);
    labCorner=slider(L"圆角大小",810,18,286,std::min(8,NoteMaximumCornerRadius()),NoteMaximumCornerRadius(),NoteCornerRadius());
    labWidth=slider(L"边缘宽度",816,18,339,0,std::min(48,NoteCornerRadius()),30);
    labBase=slider(L"基础光程",804,18,392,0,32,32);
    labRelief=slider(L"起伏高度",811,18,445,0,32,10);
    labReflection=slider(L"背景反光",823,290,180,0,100,50);
    labHighlight=slider(L"细高光",812,290,233,0,100,80);
    labRoughness=slider(L"边缘柔化",824,290,286,0,100,30);
    labEnvironmentTint=slider(L"反光染色",825,290,339,0,100,35);
    labDispersion=slider(L"色散强度",813,290,392,0,100,0);
    control(L"STATIC",L"柔化只作用于边缘。\r\n染色取自附近背景；\r\n整块底色由「遮色」控制。",0,0,290,445,250,62);
    labOpticalOnly=control(L"BUTTON",L"只看折射（保留模糊与遮色）",BS_AUTOCHECKBOX|WS_TABSTOP,822,18,514,524,26);
    labAdaptive=control(L"BUTTON",L"保持轮廓可辨（自适应明暗）",BS_AUTOCHECKBOX|WS_TABSTOP,815,18,544,524,26);
    control(L"BUTTON",L"冻结背景（便于对照）",WS_TABSTOP,805,18,584,252,30);
    control(L"BUTTON",L"推荐预设",WS_TABSTOP,807,290,584,252,30);
    control(L"BUTTON",L"找回实验便签",WS_TABSTOP,808,18,624,252,30);
    control(L"BUTTON",L"返回便签",WS_TABSTOP,806,290,624,252,30);
    control(L"BUTTON",L"准备并截图",WS_TABSTOP,814,18,664,524,30);
    labStatus=control(L"STATIC",L"正在准备背景…",0,809,18,704,524,90);
    SetTimer(labControls,1,1000,nullptr);RefreshLabControls();ShowWindow(labControls,SW_SHOWNOACTIVATE);ApplyWindowStacking();
}
}
#include "glass_experience.h"
