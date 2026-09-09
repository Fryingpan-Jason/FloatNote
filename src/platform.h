#pragma once
#include <cmath>
#include <cerrno>
#include <climits>

// Values stored in COLORREF are BGR, while labels use conventional RGB hex.
COLORREF BlendColor(COLORREF a, COLORREF b, int percent) {
    auto channel = [percent](int first, int second) { return (first * (100 - percent) + second * percent + 50) / 100; };
    return RGB(channel(GetRValue(a), GetRValue(b)), channel(GetGValue(a), GetGValue(b)),
               channel(GetBValue(a), GetBValue(b)));
}
double Luminance(COLORREF c) {
    auto linear = [](int v) {
        const double s = v / 255.0;
        return s <= 0.04045 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(GetRValue(c)) + 0.7152 * linear(GetGValue(c)) + 0.0722 * linear(GetBValue(c));
}
COLORREF ContrastText(COLORREF background) {
    const double lum = Luminance(background);
    const double dark = (lum + 0.05) / (Luminance(RGB(30, 32, 36)) + 0.05);
    const double light = (Luminance(RGB(248, 249, 250)) + 0.05) / (lum + 0.05);
    if (std::max(dark, light) >= 4.5)
        return dark >= light ? RGB(30, 32, 36) : RGB(248, 249, 250);
    return (lum + 0.05) / 0.05 >= 1.05 / (lum + 0.05) ? RGB(0, 0, 0) : RGB(255, 255, 255);
}
bool AtomicWrite(const std::filesystem::path& path, const std::string& bytes) {
    const std::filesystem::path temp = path.wstring() + L".tmp";
    HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    const bool writeOk =
        bytes.empty() || WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    const bool flushOk = FlushFileBuffers(file) != FALSE;
    const DWORD writeError = GetLastError();
    CloseHandle(file);
    if (!writeOk || !flushOk || written != bytes.size()) {
        DeleteFileW(temp.c_str());
        SetLastError(writeError);
        return false;
    }
    return MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}
bool SystemHighContrast() {
    HIGHCONTRASTW value{sizeof(value)};
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(value), &value, 0) && (value.dwFlags & HCF_HIGHCONTRASTON);
}
bool SystemTransparencyEnabled() {
    DWORD value = 1, size = sizeof(value);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"EnableTransparency", RRF_RT_REG_DWORD, nullptr, &value, &size);
    return value != 0;
}
bool SystemEnergySaver() {
    SYSTEM_POWER_STATUS power{};
    return GetSystemPowerStatus(&power) && power.SystemStatusFlag == 1;
}
bool SystemComposition() {
    BOOL enabled = FALSE;
    return SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled;
}
RECT ConstrainToWorkArea(RECT rect, RECT work) {
    const int width =
        std::clamp(static_cast<int>(rect.right - rect.left), 1, static_cast<int>(std::max(1L, work.right - work.left)));
    const int height =
        std::clamp(static_cast<int>(rect.bottom - rect.top), 1, static_cast<int>(std::max(1L, work.bottom - work.top)));
    rect.left = std::clamp(rect.left, work.left, work.right - width);
    rect.top = std::clamp(rect.top, work.top, work.bottom - height);
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;
    return rect;
}
