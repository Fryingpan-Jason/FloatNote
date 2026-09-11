#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include "capture_signal.h"
#pragma comment(lib,"oleaut32.lib")

// Capture one fully composed monitor. DWM owns transparency, window ordering,
// shell surfaces and transient overlays. The caller excludes its own note using
// WDA_EXCLUDEFROMCAPTURE before starting; it must never start without that guard.
class MonitorBackdropCapture {
    using CaptureItem=winrt::Windows::Graphics::Capture::GraphicsCaptureItem;
    using FramePool=winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool;
    CaptureItem item{nullptr};
    FramePool pool{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice native{nullptr};
    winrt::event_token arrival{};
    std::shared_ptr<CaptureSignal> signal;
    HMONITOR monitor=nullptr;
    SIZE poolSize{};
    bool subscribed=false,borderless=false;
    RECT area{};
    HRESULT Start(HWND note,ID3D11Device* device) {
        try {
            DWORD affinity=0;
            if(!GetWindowDisplayAffinity(note,&affinity) || affinity!=WDA_EXCLUDEFROMCAPTURE)
                return E_ACCESSDENIED;
            monitor=MonitorFromWindow(note,MONITOR_DEFAULTTONEAREST);
            auto factory=winrt::get_activation_factory<CaptureItem,IGraphicsCaptureItemInterop>();
            winrt::check_hresult(factory->CreateForMonitor(monitor,winrt::guid_of<CaptureItem>(),winrt::put_abi(item)));
            Microsoft::WRL::ComPtr<IDXGIDevice> dxgi; winrt::check_hresult(device->QueryInterface(IID_PPV_ARGS(&dxgi)));
            winrt::com_ptr<IInspectable> wrapped;
            winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),wrapped.put()));
            native=wrapped.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
            const auto size=item.Size(); if(size.Width<=0 || size.Height<=0)return E_INVALIDARG;
            poolSize={size.Width,size.Height};
            pool=FramePool::CreateFreeThreaded(native,
                winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,size);
            session=pool.CreateCaptureSession(item); session.IsCursorCaptureEnabled(false);
            if(signal->BorderlessAllowed()){session.IsBorderRequired(false);borderless=true;}
            if(auto interval=session.try_as<winrt::Windows::Graphics::Capture::IGraphicsCaptureSession5>())
                interval.MinUpdateInterval(std::chrono::milliseconds(16));
            arrival=pool.FrameArrived([notifications=signal](auto const&,auto const&){notifications->Notify();});
            subscribed=true;session.StartCapture();sourceChanged=true;return S_OK;
        }catch(winrt::hresult_error const& e){Close();return e.code();}
    }
public:
    bool ready=false,sourceChanged=false;
    double latestFrameAgeMs=0;
    unsigned long long captures=0,drained=0;
    ~MonitorBackdropCapture(){Close();}
    void SetSignal(const std::shared_ptr<CaptureSignal>& value){signal=value;}
    std::wstring Detail() const {
        return L"source=composed-monitor\r\nmonitorBounds="+std::to_wstring(area.left)+L","+std::to_wstring(area.top)+L","+
            std::to_wstring(area.right)+L","+std::to_wstring(area.bottom)+L"\r\nmonitorFrames="+std::to_wstring(captures);
    }
    HRESULT Poll(HWND note,ID3D11Device* device,ID3D11DeviceContext* context,
                 Microsoft::WRL::ComPtr<ID3D11Texture2D>& output,RECT& rectangle,bool& updated) {
        updated=false;sourceChanged=false;latestFrameAgeMs=0;
        if(!session){const HRESULT hr=Start(note,device);if(FAILED(hr))return hr;}
        try {
            MONITORINFO mi{sizeof(mi)}; if(!GetMonitorInfoW(monitor,&mi))return HRESULT_FROM_WIN32(GetLastError());
            if(!EqualRect(&area,&mi.rcMonitor)){area=mi.rcMonitor;sourceChanged=true;}
            if(!borderless && signal->BorderlessAllowed()){session.IsBorderRequired(false);borderless=true;}
            auto frame=pool.TryGetNextFrame(); if(!frame)return S_OK;
            for(unsigned i=0;i<3;i++){
                auto next=pool.TryGetNextFrame();if(!next)break;
                frame.Close();frame=std::move(next);++drained;
            }
            const auto size=frame.ContentSize();
            if(size.Width!=poolSize.cx || size.Height!=poolSize.cy){
                frame.Close();ready=false;
                if(size.Width<=0 || size.Height<=0)return S_OK;
                poolSize={size.Width,size.Height};
                pool.Recreate(native,winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,size);
                return S_OK;
            }
            // A display mode switch may produce one old-sized frame. Wait for
            // matching geometry instead of scaling old pixels or painting black.
            if(size.Width!=area.right-area.left || size.Height!=area.bottom-area.top){frame.Close();return S_OK;}
            auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
            Microsoft::WRL::ComPtr<ID3D11Texture2D> input;
            winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(&input)));
            D3D11_TEXTURE2D_DESC desc{},previous{};input->GetDesc(&desc);if(output)output->GetDesc(&previous);
            if(!output || previous.Width!=desc.Width || previous.Height!=desc.Height || previous.Format!=desc.Format){
                output.Reset();desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=0;desc.CPUAccessFlags=0;desc.MiscFlags=0;
                winrt::check_hresult(device->CreateTexture2D(&desc,nullptr,&output));sourceChanged=true;
            }
            context->CopyResource(output.Get(),input.Get());
            latestFrameAgeMs=std::max(0.0,GlassClockMs()-double(frame.SystemRelativeTime().count())/10000.0);
            frame.Close();rectangle=area;updated=true;ready=true;++captures;return S_OK;
        }catch(winrt::hresult_error const& e){Close();return e.code();}
    }
    void Close(){
        try{if(pool && subscribed)pool.FrameArrived(arrival);}catch(...){}
        subscribed=false;
        try{if(session)session.Close();}catch(...){}
        try{if(pool)pool.Close();}catch(...){}
        session=nullptr;pool=nullptr;item=nullptr;native=nullptr;
        ready=false;borderless=false;monitor=nullptr;poolSize={};area={};
    }
};
