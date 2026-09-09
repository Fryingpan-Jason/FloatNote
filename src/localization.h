#pragma once

#include <windows.h>

enum class UiLanguage : int {
    Automatic = 0,
    Chinese = 1,
    English = 2,
};

struct LocalizedStrings {
    const wchar_t* themeNames[5];
    const wchar_t* readTooLarge;
    const wchar_t* readFailed;
    const wchar_t* unsupportedText;
    const wchar_t* defaultNote;
    const wchar_t* tooManyCharacters;
    const wchar_t* glassOff;
    const wchar_t* glassHighContrast;
    const wchar_t* glassRemoteDesktop;
    const wchar_t* glassEnergySaver;
    const wchar_t* glassTransparencyOff;
    const wchar_t* glassCompositionOff;
    const wchar_t* glassOn;
    const wchar_t* glassFallback;
    const wchar_t* glassUnsupported;
    const wchar_t* autostartDescription;
    const wchar_t* trayPassThrough;
    const wchar_t* trayNormal;
    const wchar_t* editNow;
    const wchar_t* alwaysOnTop;
    const wchar_t* passThrough;
    const wchar_t* hideWindow;
    const wchar_t* showWindow;
    const wchar_t* backgroundOpacity;
    const wchar_t* glass;
    const wchar_t* customTheme;
    const wchar_t* autostart;
    const wchar_t* language;
    const wchar_t* exit;
    const wchar_t* autostartError;
    const wchar_t* stateOn;
    const wchar_t* stateOff;
    const wchar_t* stateSelected;
    const wchar_t* menuToggle;
    const wchar_t* settingsTitle;
    const wchar_t* glassHint;
    const wchar_t* normalHint;
    const wchar_t* loadErrorStatus;
    const wchar_t* saveErrorStatus;
    const wchar_t* settingsErrorStatus;
    const wchar_t* fontHint;
    const wchar_t* restoreEditHint;
    const wchar_t* shortcutConflictHint;
    const wchar_t* settingsWindowTitle;
    const wchar_t* sliderLabel;
    const wchar_t* pillLabel;
    const wchar_t* gripLabel;
    const wchar_t* tooltip;
    const wchar_t* saveFailureMessage;
    const wchar_t* saveFailureTitle;
    const wchar_t* languageAutomatic;
    const wchar_t* languageChinese;
    const wchar_t* languageEnglish;
    const wchar_t* textColor;
    const wchar_t* autoTextColor;
    const wchar_t* shadow;
};

inline const LocalizedStrings& ChineseStrings() {
    static const LocalizedStrings value{
        {L"暖白", L"鼠尾草", L"雾蓝", L"浅玫瑰", L"石墨"},
        L"笔记文件超过 4 MB，原文件已保留。\r\n请缩短内容后重新打开。",
        L"无法读取笔记，原文件已保留。\r\n请检查 data/note.txt 的权限后重新打开。",
        L"笔记格式或大小暂不支持，原文件已保留。\r\n请使用 UTF-8 文本，内容不超过 100 万字。",
        L"记下此刻重要的事。",
        L"笔记超过 100 万字，原文件已保留。请缩短内容后重新打开。",
        L"已关闭",
        L"高对比度模式使用纯色",
        L"远程桌面使用普通背景",
        L"节能模式暂停毛玻璃",
        L"Windows 透明效果已关闭",
        L"系统合成不可用",
        L"已开启",
        L"使用普通透明背景",
        L"此 Windows 不支持圆角毛玻璃",
        L"启动 FloatNote 常驻备忘录",
        L"FloatNote · 鼠标穿透中 · 双击恢复编辑",
        L"FloatNote · 双击显示笔记",
        L"立即编辑",
        L"窗口置顶",
        L"鼠标穿透",
        L"隐藏窗口",
        L"显示窗口",
        L"背景不透明度",
        L"毛玻璃",
        L"自定义主题色…",
        L"开机自启",
        L"语言 Language",
        L"退出",
        L"无法更新开机自启快捷方式。",
        L"已开启",
        L"已关闭",
        L"已选择",
        L"使用菜单切换",
        L"便签设置",
        L"模糊桌面 · 可调节背景不透明度",
        L"普通模式 · 自由调节背景",
        L"读取失败，原笔记已保留",
        L"笔记保存失败 · Ctrl+S 重试",
        L"设置保存失败，请检查文件权限",
        L"Ctrl + 滚轮调字号",
        L"Ctrl+Alt+E 恢复编辑",
        L"编辑快捷键被占用 · 双击托盘恢复",
        L"便签设置",
        L"背景不透明度",
        L"菜单（按住拖动）",
        L"调整窗口大小",
        L"单击设置 · 按住拖动\nCtrl+Alt+E 恢复编辑 · Ctrl+Alt+P 切换穿透",
        L"笔记暂时无法保存，窗口会保留。请检查 data 文件夹是否可写后重试。",
        L"FloatNote · 保存失败",
        L"自动",
        L"中文",
        L"English",
        L"自定义文字颜色…",
        L"自动选择文字颜色",
        L"窗口阴影"};
    return value;
}

inline const LocalizedStrings& EnglishStrings() {
    static const LocalizedStrings value{
        {L"Warm", L"Sage", L"Mist", L"Rose", L"Graphite"},
        L"The note is larger than 4 MB. The original file was kept.\r\nShorten it and reopen FloatNote.",
        L"The note could not be read. The original file was kept.\r\nCheck the permissions of data/note.txt and reopen "
        L"FloatNote.",
        L"This note format or size is unsupported. The original file was kept.\r\nUse UTF-8 text with at most one "
        L"million characters.",
        L"Write down what matters now.",
        L"The note exceeds one million characters. The original file was kept. Shorten it and reopen FloatNote.",
        L"Off",
        L"Solid color in High Contrast",
        L"Standard background in Remote Desktop",
        L"Glass paused by Energy Saver",
        L"Windows transparency effects are off",
        L"Desktop composition is unavailable",
        L"On",
        L"Using standard transparency",
        L"Rounded glass is unavailable on this Windows version",
        L"Start the FloatNote desktop memo",
        L"FloatNote · Click-through on · Double-click to edit",
        L"FloatNote · Double-click to show note",
        L"Edit now",
        L"Always on top",
        L"Mouse click-through",
        L"Hide window",
        L"Show window",
        L"Background opacity",
        L"Glass",
        L"Custom theme color…",
        L"Start with Windows",
        L"Language 语言",
        L"Exit",
        L"Could not update the startup shortcut.",
        L"On",
        L"Off",
        L"Selected",
        L"Use menu to toggle",
        L"Note settings",
        L"Blurred desktop · Adjustable background",
        L"Standard mode · Adjustable background",
        L"Read failed · Original note kept",
        L"Save failed · Press Ctrl+S to retry",
        L"Settings could not be saved · Check permissions",
        L"Ctrl + wheel changes text size",
        L"Ctrl+Alt+E restores editing",
        L"Edit shortcut unavailable · Double-click tray icon",
        L"Note settings",
        L"Background opacity",
        L"Menu (hold to drag)",
        L"Resize note",
        L"Click for settings · Hold to drag\nCtrl+Alt+E restores editing · Ctrl+Alt+P toggles click-through",
        L"The note cannot be saved right now, so the window will stay open. Check that the data folder is writable and "
        L"retry.",
        L"FloatNote · Save failed",
        L"Automatic",
        L"中文",
        L"English",
        L"Custom text color…",
        L"Automatic text color",
        L"Window shadow"};
    return value;
}

inline UiLanguage ResolveUiLanguage(UiLanguage preference) {
    if (preference != UiLanguage::Automatic)
        return preference;
    const LANGID language = GetUserDefaultUILanguage();
    return PRIMARYLANGID(language) == LANG_CHINESE ? UiLanguage::Chinese : UiLanguage::English;
}

inline const LocalizedStrings& StringsFor(UiLanguage preference) {
    return ResolveUiLanguage(preference) == UiLanguage::Chinese ? ChineseStrings() : EnglishStrings();
}
