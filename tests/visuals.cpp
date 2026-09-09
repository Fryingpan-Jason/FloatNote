// Focused visual diagnostics. All windows and data belong to this test process.
#include "visual_test_support.h"

int backdropKind = 0;
LRESULT CALLBACK WhiteFixture(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    if (backdropKind == 1)
        return FixtureProc(window, message, wp, lp);
    if (message == WM_PAINT) {
        PAINTSTRUCT ps{};
        auto dc = BeginPaint(window, &ps);
        RECT r{};
        GetClientRect(window, &r);
        FillRect(dc, &r, static_cast<HBRUSH>(GetStockObject(backdropKind == 2 ? BLACK_BRUSH : WHITE_BRUSH)));
        EndPaint(window, &ps);
        return 0;
    }
    return DefWindowProcW(window, message, wp, lp);
}

int RunVisualChecks() {
    std::cout << std::unitbuf;
    SmoothGraphicsRuntime graphics;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    g_instance = GetModuleHandleW(nullptr);
    g_dataDirectory = ExecutableDirectory() / L"visual-evidence" / std::to_wstring(GetTickCount64());
    std::filesystem::create_directories(g_dataDirectory);
    g_notePath = g_dataDirectory / L"note.txt";
    g_settingsPath = g_dataDirectory / L"settings.ini";
    Require(AtomicWrite(g_notePath, "Desktop memo\r\nReview notes"), "isolated visual fixture note");
    WNDCLASSW bg{};
    bg.hInstance = g_instance;
    bg.lpfnWndProc = WhiteFixture;
    bg.lpszClassName = L"FloatNote.VisualWhite";
    RegisterClassW(&bg);
    HWND fixture = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, bg.lpszClassName, L"Visual fixture",
                                  WS_POPUP, 100, 100, 800, 500, nullptr, nullptr, g_instance, nullptr);
    ShowWindow(fixture, SW_SHOWNOACTIVATE);
    WNDCLASSW note{};
    note.hInstance = g_instance;
    note.lpfnWndProc = NoteProc;
    note.lpszClassName = L"FloatNote.VisualNote";
    RegisterClassW(&note);
    g_settings = Settings{};
    g_settings.glass = true;
    g_settings.topmost = true;
    CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST, note.lpszClassName, L"Visual note",
                    WS_POPUP | WS_CLIPCHILDREN, 180, 180, 640, 320, fixture, nullptr, g_instance, nullptr);
    ApplyInteractionMode();
    ShowWindow(g_window, SW_SHOWNOACTIVATE);
    for (int mode = 0; mode < 4; ++mode) {
        g_settings.shadow = mode % 2 == 0;
        g_settings.opacityPercent = 0;
        ApplyVisuals(g_window);
        ShowWindow(g_window, SW_SHOWNOACTIVATE);
        RedrawWindow(fixture, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        SetWindowPos(g_window, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        RoundWindow(g_window, kCornerRadius);
        RequestRender();
        PumpFor(350);
        DwmFlush();
        auto pixels = CaptureWindow(fixture, g_dataDirectory / (L"shadow-" + std::to_wstring(mode) + L".png"));
        const auto outside = pixels[420 * 800 + 400] & 0xffffff;
        Require(mode % 2 ? outside == 0xffffff : outside < 0xf0f0f0, "shadow toggle changes actual exterior pixels");
    }
    backdropKind = 1;
    RedrawWindow(fixture, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    g_settings.glass = false;
    SetOpacityPercent(0);
    PumpFor(250);
    const auto plain = Capture(g_dataDirectory / L"plain-stripes.png");
    g_settings.glass = true;
    ApplyVisuals(g_window);
    PumpFor(250);
    const auto glass = Capture(g_dataDirectory / L"glass-no-shadow.png");
    Require(g_glassActive && StripeContrast(glass, 640) < StripeContrast(plain, 640) * 0.6,
            "shadow-free glass still blurs real desktop details");
    for (POINT p : {POINT{2, 2}, POINT{637, 2}, POINT{2, 317}, POINT{637, 317}})
        Require((glass[p.y * 640 + p.x] & 0xffffff) == (plain[p.y * 640 + p.x] & 0xffffff),
                "shadow-free glass leaves rounded corners untouched");
    SetOpacityPercent(100);
    PumpFor(150);
    const auto solid = Capture(g_dataDirectory / L"glass-opaque.png");
    Require(StripeContrast(solid, 640) < 0.2, "glass 100 percent hides desktop stripes");
    SetOpacityPercent(45);
    PumpFor(150);
    Capture(g_dataDirectory / L"glass-tinted.png");
    backdropKind = 2;
    RedrawWindow(fixture, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    g_settings.themeColor = RGB(243, 222, 227);
    g_settings.autoTextColor = false;
    g_settings.textColor = RGB(30, 32, 36);
    SetOpacityPercent(0);
    RefreshTheme();
    SetFocus(nullptr);
    PumpFor(200);
    Capture(g_dataDirectory / L"black-text-on-black.png");
    RECT e{};
    GetWindowRect(g_edit, &e);
    MapWindowPoints(nullptr, g_window, reinterpret_cast<POINT*>(&e), 2);
    int edgeCount = 0;
    bool noResidue = true;
    for (int y = e.top; y < e.bottom; ++y)
        for (int x = e.left; x < e.right; ++x) {
            auto p = g_surface.pixels[y * g_surface.width + x];
            noResidue &= ((p >> 16) & 255) <= 31 && ((p >> 8) & 255) <= 33 && (p & 255) <= 37;
            edgeCount += (p >> 24) > 1 && (p >> 24) < 255;
        }
    Require(noResidue, "dark text has no light matte residue");
    Require(edgeCount > 20, "text retains smooth fractional-alpha edges");
    SetForegroundWindow(g_window);
    SetFocus(g_edit);
    SendMessageW(g_edit, EM_SETSEL, 0, 4);
    PumpFor(100);
    Capture(g_dataDirectory / L"native-selection.png");
    int selectionPixels = 0;
    const auto highlight = PremultiplyPixel(GetSysColor(COLOR_HIGHLIGHT), 255);
    for (int y = e.top; y < e.bottom; ++y)
        for (int x = e.left; x < e.right; ++x)
            selectionPixels += g_surface.pixels[y * g_surface.width + x] == highlight;
    Require(selectionPixels > 50, "native selection background survives coverage masks");
    SendMessageW(g_edit, EM_SETSEL, 0, 0);
    SetFocus(nullptr);
    SetPassThrough(true);
    PumpFor(150);
    Require(WindowFromPoint({600, 430}) == fixture, "shadow-free glass passes input through");
    SetPassThrough(false);
    g_settings.textColor = RGB(255, 255, 255);
    RefreshTheme();
    PumpFor(150);
    Capture(g_dataDirectory / L"white-text-on-black.png");
    g_settings.textColor = RGB(65, 190, 255);
    RefreshTheme();
    PumpFor(150);
    Capture(g_dataDirectory / L"custom-text-on-black.png");
    g_settings.opacityPercent = 70;
    ApplyVisuals(g_window);
    TogglePillMenu();
    PumpFor(150);
    CaptureWindow(g_menu, g_dataDirectory / L"settings.png");
    Require(IsWindowVisible(g_slider), "background slider remains available in glass mode");
    HandleMenuCommand(kMenuLanguageChinese);
    PumpFor(100);
    CaptureWindow(g_menu, g_dataDirectory / L"settings-chinese.png");
    CloseMenu();
    SaveSettings();
    g_settings = Settings{};
    LoadSettings();
    Require(!g_settings.autoTextColor && g_settings.textColor == RGB(65, 190, 255) && !g_settings.shadow &&
            g_settings.opacityPercent == 70, "custom ink, opacity and shadow survive reload");
    HandleMenuCommand(kMenuAutoTextColor);
    Require(g_settings.autoTextColor && kNoteText == ContrastText(g_settings.themeColor), "automatic ink can be restored");
    g_settings.themeColor = RGB(0, 0, 0);
    RefreshTheme();
    Require(kNoteText == ContrastText(RGB(0, 0, 0)) && GetRValue(kNoteText) > 200,
            "black theme still chooses light text in automatic mode");
    std::wcout << L"Evidence: " << g_dataDirectory << L'\n';
    SendMessageW(g_window, WM_CLOSE, 0, 0);
    DestroyWindow(fixture);
    return 0;
}

int main() {
    try {
        return RunVisualChecks();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (IsWindow(g_window))
            SendMessageW(g_window, WM_CLOSE, 0, 0);
        return 1;
    }
}
