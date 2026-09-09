# FloatNote 1.1.0

## What's new

- Turn the window shadow on or off while retaining rounded glass.
- Choose a custom text color, or return to automatic theme-based text color in settings.
- Adjust background opacity from 0–100% in both standard and glass modes. Zero shows the desktop (blurred in glass mode); 100% covers it with the theme color. Text stays opaque.
- Fix light fringes around text over dark backgrounds while preserving native text selection and editing.

Your existing theme and automatic text-color behavior are retained. Glass now uses your saved background opacity; set it to 0% for glass without an added theme-color layer.

Choose `windows-x64` for most Intel/AMD PCs, `windows-x86` for 32-bit Windows 10, or `windows-arm64` for Windows 11 ARM devices. To upgrade, exit FloatNote and replace the old `FloatNote.exe` with the new one in the same folder. Keep the existing `data` folder to preserve your note and settings. The release archives contain no user data.

Windows 10 uses standard transparency. Glass requires supported Windows 11 and Windows transparency effects; it falls back in High Contrast, Energy Saver, and Remote Desktop. Binaries are unsigned, so Windows may show a SmartScreen warning. SHA-256 checksums are included in `SHA256SUMS.txt`.

---

## 本次更新

- 新增窗口阴影开关，关闭阴影时保留圆角和毛玻璃。
- 新增自定义文字颜色，也可在设置中恢复自动按主题色选择文字颜色。
- 普通模式和毛玻璃模式均支持 0–100% 背景不透明度：0% 透出桌面（毛玻璃模式下为模糊桌面），100% 为纯主题色；文字不会随背景变淡。
- 修复深色背景上的文字浅色毛边，保留原生文字选区和编辑行为。

升级后保留原有主题及自动文字颜色。毛玻璃现在会使用已保存的背景不透明度；希望不叠加主题色时，将其设为 0%。

大多数 Intel/AMD 电脑请选择 `windows-x64`；32 位 Windows 10 请选择 `windows-x86`；Windows 11 ARM 设备请选择 `windows-arm64`。升级时先退出 FloatNote，将新版 `FloatNote.exe` 替换到原目录，保留原有 `data` 文件夹即可保留笔记和设置。发布压缩包不含用户数据。

Windows 10 使用普通透明。毛玻璃需要支持的 Windows 11 和系统透明效果；高对比度、节能模式或远程桌面下会自动回退。程序未签名，Windows 可能显示 SmartScreen 提示。校验值见 `SHA256SUMS.txt`。
