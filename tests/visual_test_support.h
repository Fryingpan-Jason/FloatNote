#pragma once

// Bounded compositor checks, using only temporary notes and test windows.
#define FLOATNOTE_DIAGNOSTICS
#include "../src/main.cpp"
#include <iostream>
#include <stdexcept>

void Require(bool condition, const char* message) {
    std::cout << (condition ? "PASS " : "FAIL ") << message << '\n';
    if (!condition)
        throw std::runtime_error(message);
}
void PumpFor(DWORD milliseconds) {
    const auto end = GetTickCount64() + milliseconds;
    while (GetTickCount64() < end) {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT)
                return;
            if (HandleAppKey(message))
                continue;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}
LRESULT CALLBACK FixtureProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(h, &ps);
        RECT area{};
        GetClientRect(h, &area);
        for (int x = 0; x < area.right; x += 8) {
            const int shade = (x / 8) % 2 ? 25 : -25;
            HBRUSH color = CreateSolidBrush(x < area.right / 2 ? RGB(195 + shade, 60 + shade, 75 + shade)
                                                               : RGB(45 + shade, 105 + shade, 210 + shade));
            RECT strip{x, 0, x + 8, area.bottom};
            FillRect(dc, &strip, color);
            DeleteObject(color);
        }
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
LRESULT CALLBACK NoteProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CREATE) {
        g_window = h;
        CreateControls(h);
        LoadNote();
        return 0;
    }
    return WindowProcedure(h, m, w, l);
}
std::vector<DWORD> CaptureWindow(HWND window, const std::filesystem::path& file) {
    RECT r{};
    GetWindowRect(window, &r);
    SurfaceBuffer capture;
    Require(capture.Resize(r.right - r.left, r.bottom - r.top), "screen capture buffer");
    HDC screen = GetDC(nullptr);
    Require(BitBlt(capture.dc, 0, 0, capture.width, capture.height, screen, r.left, r.top, SRCCOPY | CAPTUREBLT) !=
                FALSE,
            "screen compositor capture");
    ReleaseDC(nullptr, screen);
    std::vector<DWORD> pixels(capture.pixels, capture.pixels + capture.width * capture.height);
    for (auto& p : pixels)
        p |= 0xff000000;
    Gdiplus::Bitmap png(capture.width, capture.height, capture.width * 4, PixelFormat32bppARGB,
                        reinterpret_cast<BYTE*>(pixels.data()));
    CLSID encoder{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
    Require(png.Save(file.c_str(), &encoder, nullptr) == Gdiplus::Ok, "PNG evidence written");
    return pixels;
}
std::vector<DWORD> Capture(const std::filesystem::path& file) {
    return CaptureWindow(g_window, file);
}
double StripeContrast(const std::vector<DWORD>& pixels, int width) {
    double sum = 0;
    int count = 0;
    for (int y = 190; y < 260; ++y)
        for (int x = 50; x < width - 50; ++x) {
            const DWORD a = pixels[y * width + x], b = pixels[y * width + x - 1];
            sum += abs(int((a >> 16) & 255) - int((b >> 16) & 255)) + abs(int(a & 255) - int(b & 255));
            ++count;
        }
    return sum / count;
}
ULONGLONG CpuTicks() {
    FILETIME created{}, exited{}, kernel{}, user{};
    GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user);
    ULARGE_INTEGER k{}, u{};
    k.LowPart = kernel.dwLowDateTime;
    k.HighPart = kernel.dwHighDateTime;
    u.LowPart = user.dwLowDateTime;
    u.HighPart = user.dwHighDateTime;
    return k.QuadPart + u.QuadPart;
}
