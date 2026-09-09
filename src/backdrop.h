#pragma once
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include <roapi.h>
#include <windows.ui.composition.h>
#include <windows.ui.composition.desktop.h>
#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>
#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "CoreMessaging.lib")

struct NativeBackdrop {
    Microsoft::WRL::ComPtr<ABI::Windows::System::IDispatcherQueueController> queue;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositor> compositor;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionTarget> target;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::IVisual> visual;
    bool initialized = false;
    bool attempted = false;
    bool ready = false;
    HRESULT error = S_OK;
    bool Enable(HWND window, bool enabled) {
        namespace c = ABI::Windows::UI::Composition;
        using Microsoft::WRL::ComPtr;
        if (!enabled) {
            if (visual)
                visual->put_IsVisible(FALSE);
            return true;
        }
        const BOOL host = TRUE;
        if (FAILED(error = DwmSetWindowAttribute(window, static_cast<DWMWINDOWATTRIBUTE>(17), &host, sizeof(host))))
            return false;
        if (!ready) {
            if (attempted)
                return false;
            attempted = true;
            error = RoInitialize(RO_INIT_SINGLETHREADED);
            if (FAILED(error))
                return false;
            initialized = true;
            DispatcherQueueOptions options{sizeof(options), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA};
            if (FAILED(error = CreateDispatcherQueueController(options, &queue)))
                return false;
            ComPtr<IInspectable> instance;
            if (FAILED(error = RoActivateInstance(
                           Microsoft::WRL::Wrappers::HStringReference(L"Windows.UI.Composition.Compositor").Get(),
                           &instance)))
                return false;
            if (FAILED(error = instance.As(&compositor)))
                return false;
            ComPtr<c::Desktop::ICompositorDesktopInterop> interop;
            if (FAILED(error = compositor.As(&interop)))
                return false;
            ComPtr<c::Desktop::IDesktopWindowTarget> desktop;
            if (FAILED(error = interop->CreateDesktopWindowTarget(window, FALSE, &desktop)))
                return false;
            if (FAILED(error = desktop.As(&target)))
                return false;
            ComPtr<c::ICompositor3> compositor3;
            compositor.As(&compositor3);
            if (!compositor3)
                return false;
            ComPtr<c::ICompositionBackdropBrush> backdrop;
            if (FAILED(error = compositor3->CreateHostBackdropBrush(&backdrop)))
                return false;
            ComPtr<c::ICompositionBrush> brush;
            backdrop.As(&brush);
            ComPtr<c::ISpriteVisual> sprite;
            if (FAILED(error = compositor->CreateSpriteVisual(&sprite)))
                return false;
            sprite->put_Brush(brush.Get());
            sprite.As(&visual);
            if (FAILED(error = target->put_Root(visual.Get())))
                return false;
            ready = true;
        }
        RECT rect{};
        GetClientRect(window, &rect);
        visual->put_Size({static_cast<float>(rect.right), static_cast<float>(rect.bottom)});
        return SUCCEEDED(error = visual->put_IsVisible(TRUE));
    }
    void Resize(int width, int height) {
        if (visual)
            visual->put_Size({static_cast<float>(width), static_cast<float>(height)});
    }
    void Close() {
        if (target)
            target->put_Root(nullptr);
        visual.Reset();
        target.Reset();
        compositor.Reset();
        queue.Reset();
        if (initialized) {
            RoUninitialize();
            initialized = false;
        }
        ready = false;
        attempted = false;
    }
};
