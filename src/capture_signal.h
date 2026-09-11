#pragma once
#include <atomic>
#include <memory>
#include <chrono>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>

inline constexpr UINT kGlassFrameReady=WM_APP+71;

inline double GlassClockMs() {
    static const double frequency=[] { LARGE_INTEGER f{}; QueryPerformanceFrequency(&f); return double(f.QuadPart); }();
    LARGE_INTEGER now{}; QueryPerformanceCounter(&now); return now.QuadPart*1000.0/frequency;
}

struct CaptureSignal {
    HWND window=nullptr;
    std::atomic<bool> alive{true},posted{false};
    std::atomic<double> notifiedAt{0};
    std::atomic<int> borderAccess{-1};
    void Notify() {
        if(!alive.load() || !window) return;
        bool expected=false;
        if(posted.compare_exchange_strong(expected,true)) {
            notifiedAt.store(GlassClockMs());
            if(!PostMessageW(window,kGlassFrameReady,0,0)) posted.store(false);
        }
    }
    bool BorderlessAllowed() const {
        using winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus;
        return borderAccess.load()==int(AppCapabilityAccessStatus::Allowed);
    }
};

// Out-of-context accessibility notifications: no injection and no window mutation.
class CaptureWindowEvents {
    static inline std::weak_ptr<CaptureSignal> active;
    HWINEVENTHOOK locations=nullptr,foreground=nullptr;
    static void CALLBACK OnEvent(HWINEVENTHOOK,DWORD event,HWND window,LONG object,LONG,DWORD,DWORD) {
        if(!window || (event!=EVENT_SYSTEM_FOREGROUND && object!=OBJID_WINDOW)) return;
        if(auto signal=active.lock()) signal->Notify();
    }
public:
    void Start(const std::shared_ptr<CaptureSignal>& signal) {
        if(locations) return;
        active=signal;
        locations=SetWinEventHook(EVENT_OBJECT_CREATE,EVENT_OBJECT_LOCATIONCHANGE,nullptr,OnEvent,0,0,
                                 WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
        foreground=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,OnEvent,0,0,
                                  WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
    }
    void Stop() {
        if(locations) UnhookWinEvent(locations);
        if(foreground) UnhookWinEvent(foreground);
        locations=foreground=nullptr; active.reset();
    }
    ~CaptureWindowEvents(){Stop();}
};
