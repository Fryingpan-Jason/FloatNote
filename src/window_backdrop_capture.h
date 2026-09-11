#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <memory>
#include "capture_signal.h"
#pragma comment(lib,"oleaut32.lib")

// Each input is another window's own surface, including the Explorer desktop.
// Never capture the composed monitor or hide the note from remote capture.
class WindowBackdropCapture {
    struct Source {
        HWND window=nullptr;
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool pool{nullptr};
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice captureDevice{nullptr};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        SIZE size{};
        winrt::event_token arrival{};
        bool subscribed=false,borderless=false;
        std::shared_ptr<CaptureSignal> signal;
        double frameAgeMs=0;
        unsigned drainedFrames=0;
        ~Source() { Close(); }
        void Close() {
            try { if(pool && subscribed) pool.FrameArrived(arrival); } catch(...) {}
            subscribed=false;
            try { if(session) session.Close(); } catch(...) {}
            try { if(pool) pool.Close(); } catch(...) {}
            session=nullptr; pool=nullptr; item=nullptr; captureDevice=nullptr; texture.Reset();
        }
        HRESULT Start(ID3D11Device* device,HWND target,const std::shared_ptr<CaptureSignal>& notifications) {
            try {
                window=target;
                signal=notifications;
                auto factory=winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
                winrt::check_hresult(factory->CreateForWindow(window,
                    winrt::guid_of<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>(),winrt::put_abi(item)));
                Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;
                winrt::check_hresult(device->QueryInterface(IID_PPV_ARGS(&dxgi)));
                winrt::com_ptr<IInspectable> wrapped;
                winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),wrapped.put()));
                captureDevice=wrapped.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
                const auto initial=item.Size(); size={initial.Width,initial.Height};
                if(size.cx<=0 || size.cy<=0) return E_INVALIDARG;
                pool=winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
                    captureDevice,winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,initial);
                session=pool.CreateCaptureSession(item); session.IsCursorCaptureEnabled(false);
                if(signal->BorderlessAllowed()) { session.IsBorderRequired(false); borderless=true; }
                // Let the capture producer pace frames. The UI consumes the newest
                // available frame instead of polling an older buffered frame.
                if(auto interval=session.try_as<winrt::Windows::Graphics::Capture::IGraphicsCaptureSession5>())
                    interval.MinUpdateInterval(std::chrono::milliseconds(16));
                arrival=pool.FrameArrived([notifications](auto const&,auto const&) {notifications->Notify();}); subscribed=true;
                session.StartCapture(); return S_OK;
            } catch(const winrt::hresult_error& e) { Close(); return e.code(); }
        }
        HRESULT Poll(ID3D11Device* device,ID3D11DeviceContext* context,bool& updated) {
            try {
                if(!borderless && signal->BorderlessAllowed()) { session.IsBorderRequired(false); borderless=true; }
                auto frame=pool.TryGetNextFrame();
                if(!frame) return S_OK;
                for(unsigned i=0;i<3;i++) {
                    auto next=pool.TryGetNextFrame();
                    if(!next) break;
                    frame.Close(); frame=std::move(next); ++drainedFrames;
                }
                frameAgeMs=std::max(0.0,GlassClockMs()-double(frame.SystemRelativeTime().count())/10000.0);
                auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
                Microsoft::WRL::ComPtr<ID3D11Texture2D> input;
                winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(&input)));
                D3D11_TEXTURE2D_DESC desc{},current{}; input->GetDesc(&desc);
                if(texture) texture->GetDesc(&current);
                if(!texture || current.Width!=desc.Width || current.Height!=desc.Height || current.Format!=desc.Format) {
                    texture.Reset(); desc.Usage=D3D11_USAGE_DEFAULT; desc.BindFlags=0; desc.CPUAccessFlags=0; desc.MiscFlags=0;
                    winrt::check_hresult(device->CreateTexture2D(&desc,nullptr,&texture));
                }
                context->CopyResource(texture.Get(),input.Get()); updated=true;
                const auto next=frame.ContentSize(); frame.Close();
                if(next.Width>0 && next.Height>0 && (next.Width!=size.cx || next.Height!=size.cy)) {
                    size={next.Width,next.Height};
                    pool.Recreate(captureDevice,winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,next);
                }
                return S_OK;
            } catch(const winrt::hresult_error& e) { return e.code(); }
        }
    };
    struct Layer { HWND window; RECT bounds; std::vector<RECT> fragments; };
    struct Region { HRGN value; explicit Region(HRGN r):value(r){} ~Region(){if(value)DeleteObject(value);} };
    std::vector<Layer> plan;
    std::vector<std::unique_ptr<Source>> sources;
    RECT area{};
    ULONGLONG lastScan=0;
    bool validPlan=false;
    std::wstring diagnostic;
    std::shared_ptr<CaptureSignal> notifications;
    std::vector<double> geometryErrors;
    static RECT Bounds(HWND window) {
        RECT r{};
        if(FAILED(DwmGetWindowAttribute(window,DWMWA_EXTENDED_FRAME_BOUNDS,&r,sizeof(r)))) GetWindowRect(window,&r);
        return r;
    }
    static bool ShellSurface(HWND window) {
        wchar_t cls[128]{}; GetClassNameW(window,cls,128);
        return wcscmp(cls,L"Progman")==0 || wcscmp(cls,L"WorkerW")==0;
    }
    static bool AppendLayer(std::vector<Layer>& layers,HWND window,const RECT& bounds,HRGN fragments) {
        const DWORD size=GetRegionData(fragments,0,nullptr);
        if(!size) return false;
        std::vector<BYTE> bytes(size); auto data=reinterpret_cast<RGNDATA*>(bytes.data());
        if(!GetRegionData(fragments,size,data)) return false;
        const RECT* rects=reinterpret_cast<const RECT*>(data->Buffer);
        layers.push_back({window,bounds,{rects,rects+data->rdh.nCount}}); return true;
    }
    static bool FillDesktop(std::vector<Layer>& layers,HRGN remaining,std::wstring& reason) {
        // Explorer's shell window includes its child visual tree: icons and
        // wallpaper hosts (including a Wallpaper Engine child on this machine).
        // It is deliberately sampled as a window, not as the complete screen.
        // Other shell layouts may place the icon host in a separate WorkerW.
        std::vector<HWND> desktops;
        if(HWND shell=GetShellWindow(); shell && IsWindowVisible(shell)) desktops.push_back(shell);
        for(HWND w=GetTopWindow(nullptr);w;w=GetWindow(w,GW_HWNDNEXT)) {
            if(!IsWindowVisible(w) || !ShellSurface(w) ||
               std::find(desktops.begin(),desktops.end(),w)!=desktops.end()) continue;
            if(FindWindowExW(w,nullptr,L"SHELLDLL_DefView",nullptr)) desktops.insert(desktops.begin(),w);
        }
        Region intersection(CreateRectRgn(0,0,0,0));
        for(HWND w:desktops) {
            const RECT bounds=Bounds(w); Region region(CreateRectRgnIndirect(&bounds));
            const int kind=CombineRgn(intersection.value,remaining,region.value,RGN_AND);
            if(kind==ERROR) return false;
            if(kind==NULLREGION) continue;
            DWORD affinity=0;
            if(GetWindowDisplayAffinity(w,&affinity) && affinity!=WDA_NONE) return false;
            if(!AppendLayer(layers,w,bounds,intersection.value)) return false;
            reason+=L" desktop-shell="+std::to_wstring(reinterpret_cast<UINT_PTR>(w));
            if(CombineRgn(remaining,remaining,region.value,RGN_DIFF)==NULLREGION) return true;
        }
        reason+=L" desktop surface unavailable";return false;
    }
    static std::vector<Layer> FindBackground(HWND note,const RECT& rectangle,std::wstring& reason) {
        std::vector<Layer> result;
        reason=L"note="+std::to_wstring(rectangle.left)+L","+std::to_wstring(rectangle.top)+L","+std::to_wstring(rectangle.right)+L","+std::to_wstring(rectangle.bottom);
        Region remaining(CreateRectRgnIndirect(&rectangle)), intersection(CreateRectRgn(0,0,0,0));
        unsigned visited=0,visible=0,other=0,ordinary=0,uncloaked=0;
        // Reconstruct the visible scene with this process omitted. Starting at
        // the desktop's top window also works when the launcher starts the note
        // behind other apps; any windows actually above it already occlude it.
        for(HWND w=GetTopWindow(nullptr);w;w=GetWindow(w,GW_HWNDNEXT)) {
            ++visited;
            if(!IsWindowVisible(w) || IsIconic(w)) continue;
            ++visible;
            DWORD process=0; GetWindowThreadProcessId(w,&process);
            if(process==GetCurrentProcessId()) continue;
            ++other;
            // The shell is a background layer even when it is a tool window.
            // Defer it until all ordinary application fragments are assigned.
            if(ShellSurface(w)) continue;
            if(GetWindowLongPtrW(w,GWL_EXSTYLE) & (WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW)) continue;
            ++ordinary;
            DWORD cloaked=0; DwmGetWindowAttribute(w,DWMWA_CLOAKED,&cloaked,sizeof(cloaked)); if(cloaked) continue;
            ++uncloaked;
            const RECT bounds=Bounds(w); Region windowRegion(CreateRectRgnIndirect(&bounds));
            const int kind=CombineRgn(intersection.value,remaining.value,windowRegion.value,RGN_AND);
            if(kind==NULLREGION) continue;
            if(kind==ERROR) return {};
            wchar_t cls[128]{}; GetClassNameW(w,cls,128);
            reason+=L" hit="+std::wstring(cls)+L"["+std::to_wstring(bounds.left)+L","+std::to_wstring(bounds.top)+L","+std::to_wstring(bounds.right)+L","+std::to_wstring(bounds.bottom)+L"]";
            DWORD affinity=0;
            if(GetWindowDisplayAffinity(w,&affinity) && affinity!=WDA_NONE) {reason+=L" protected";return {};}
            if(result.size()>=4) return {};
            if(!AppendLayer(result,w,bounds,intersection.value)) return {};
            if(CombineRgn(remaining.value,remaining.value,windowRegion.value,RGN_DIFF)==NULLREGION) return result;
        }
        // Fill only uncovered fragments. A card spanning an app and the desktop
        // therefore uses one continuous material instead of falling back in full.
        if(FillDesktop(result,remaining.value,reason)) return result;
        reason+=L" counters="+std::to_wstring(visited)+L","+std::to_wstring(visible)+L","+std::to_wstring(other)+L","+std::to_wstring(ordinary)+L","+std::to_wstring(uncloaked)+L" hwnd="+std::to_wstring(reinterpret_cast<UINT_PTR>(note));
        return {};
    }
    static bool SamePlan(const std::vector<Layer>& a,const std::vector<Layer>& b) {
        if(a.size()!=b.size()) return false;
        for(size_t i=0;i<a.size();i++) {
            if(a[i].window!=b[i].window || !EqualRect(&a[i].bounds,&b[i].bounds) || a[i].fragments.size()!=b[i].fragments.size()) return false;
            for(size_t j=0;j<a[i].fragments.size();j++) if(!EqualRect(&a[i].fragments[j],&b[i].fragments[j])) return false;
        }
        return true;
    }
    Source* Find(HWND window) { for(auto& s:sources) if(s->window==window) return s.get(); return nullptr; }
public:
    bool ready=false, sourceChanged=false;
    double latestFrameAgeMs=0;
    unsigned long long captures=0,drained=0;
    void SetSignal(const std::shared_ptr<CaptureSignal>& signal) {notifications=signal;}
    size_t Count() const { return sources.size(); }
    bool HasDesktop() const { return std::any_of(plan.begin(),plan.end(),[](const Layer& layer){return ShellSurface(layer.window);}); }
    std::wstring Detail() const {
        auto values=geometryErrors;std::sort(values.begin(),values.end());
        const double p95=values.empty()?0:values[std::min(values.size()-1,size_t(std::ceil(values.size()*.95)-1))];
        return diagnostic+L"\r\ngeometryErrorP95Px="+std::to_wstring(p95);
    }
    HRESULT Poll(HWND note,ID3D11Device* device,ID3D11DeviceContext* context,
                 Microsoft::WRL::ComPtr<ID3D11Texture2D>& output,RECT& rectangle,bool& updated) {
        updated=false; sourceChanged=false;
        RECT noteRect{}; GetWindowRect(note,&noteRect);
        const ULONGLONG now=GetTickCount64();
        // Called by frame/location notifications; source geometry must be current
        // on every draw, including when a source window moves without repainting.
        {
            std::wstring reason;
            auto next=FindBackground(note,noteRect,reason);
            const bool changed=!SamePlan(plan,next) || !EqualRect(&noteRect,&area);
            if(changed) {
                ready=false; sourceChanged=true;
                std::erase_if(sources,[&](const auto& source){
                    return std::none_of(next.begin(),next.end(),[&](const Layer& layer){return layer.window==source->window;});
                });
                for(auto& layer:next) if(!Find(layer.window)) {
                    auto source=std::make_unique<Source>();
                    const HRESULT hr=source->Start(device,layer.window,notifications);
                    if(FAILED(hr)) {
                        wchar_t cls[128]{};GetClassNameW(layer.window,cls,128);
                        diagnostic=L"capture start failed: "+std::wstring(cls)+L" hwnd="+std::to_wstring(reinterpret_cast<UINT_PTR>(layer.window));
                        Close(); return hr;
                    }
                    sources.push_back(std::move(source));
                }
                plan=std::move(next); area=noteRect;
            }
            validPlan=!plan.empty(); lastScan=now;
            diagnostic=L"layers="+std::to_wstring(plan.size())+L" sources="+std::to_wstring(sources.size())+L" "+reason;
        }
        if(!validPlan) return S_FALSE;
        double geometryError=0;
        for(const auto& layer:plan){const RECT actual=Bounds(layer.window);geometryError=std::max(geometryError,double(std::max(abs(actual.left-layer.bounds.left),abs(actual.top-layer.bounds.top))));}
        geometryErrors.push_back(geometryError);if(geometryErrors.size()>512)geometryErrors.erase(geometryErrors.begin());
        bool changed=sourceChanged;
        double age=0;
        for(auto& source:sources) {
            bool newFrame=false;
            const HRESULT hr=source->Poll(device,context,newFrame);
            if(FAILED(hr)) { Close(); return hr; }
            if(newFrame) {changed=true; ++captures; age=std::max(age,source->frameAgeMs); drained+=source->drainedFrames; source->drainedFrames=0;}
            if(!source->texture) { diagnostic+=L" waiting for frame"; return S_OK; }
        }
        latestFrameAgeMs=age;
        if(!changed && ready) return S_OK;
        const UINT w=area.right-area.left,h=area.bottom-area.top;
        D3D11_TEXTURE2D_DESC desc{}; if(output) output->GetDesc(&desc);
        if(!output || desc.Width!=w || desc.Height!=h) {
            output.Reset(); desc={}; desc.Width=w; desc.Height=h; desc.MipLevels=desc.ArraySize=1;
            desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count=1; desc.Usage=D3D11_USAGE_DEFAULT;
            const HRESULT hr=device->CreateTexture2D(&desc,nullptr,&output); if(FAILED(hr)) return hr;
        }
        for(const auto& layer:plan) {
            auto source=Find(layer.window); D3D11_TEXTURE2D_DESC sd{}; source->texture->GetDesc(&sd);
            for(const RECT& fragment:layer.fragments) {
                const int x=fragment.left-layer.bounds.left,y=fragment.top-layer.bounds.top;
                const int right=x+fragment.right-fragment.left,bottom=y+fragment.bottom-fragment.top;
                // Wait for a correctly sized frame after source-window resizing.
                if(x<0 || y<0 || right>int(sd.Width) || bottom>int(sd.Height)) {
                    diagnostic=L"source bounds mismatch: crop="+std::to_wstring(x)+L","+std::to_wstring(y)+L","+std::to_wstring(right)+L","+std::to_wstring(bottom)+L" texture="+std::to_wstring(sd.Width)+L"x"+std::to_wstring(sd.Height);
                    ready=false; return S_FALSE;
                }
                D3D11_BOX box{UINT(x),UINT(y),0,UINT(right),UINT(bottom),1};
                context->CopySubresourceRegion(output.Get(),0,fragment.left-area.left,fragment.top-area.top,0,source->texture.Get(),0,&box);
            }
        }
        rectangle=area; updated=true; ready=true; return S_OK;
    }
    void Close() { sources.clear(); plan.clear(); validPlan=false; ready=false; lastScan=0; area={}; }
};
