# FloatNote 2.0.0

## 下载即用

下载下方 Assets 中的 ZIP，解压后运行 `FloatNote.exe`，无需源码或开发环境。

- **大多数电脑：FloatNote-2.0.0-windows-x64.zip**
- Windows ARM 电脑：FloatNote-2.0.0-windows-arm64.zip
- 32 位环境：FloatNote-2.0.0-windows-x86.zip

### 本次更新

- 新增液态玻璃材质：连续边缘折射、自适应反光、边缘柔化与可调色散。
- 重新设计顶部操作区和设置页，支持紧凑展开、颜色预设/自定义、置顶和鼠标穿透。
- 缩小高度可收纳便签，带连续吸入、恢复和弹性反馈。
- 关闭时可选择隐藏到托盘或退出，并记住偏好。
- 保留本地自动保存、原生编辑、全局快捷键和开机启动。

升级请先退出旧版，将新版 EXE 替换到原目录，**保留 data 文件夹**。发布包不含用户数据。建议升级前备份 data。

液态玻璃在支持的 Windows 11 环境使用；远程桌面、节能、高对比度或 API 不可用时会降级。实时液态玻璃便签可能不出现在截图/第三方串流中，需要时切换毛玻璃。新设置和材质面板目前为中文。程序未签名，校验值见 `SHA256SUMS.txt`。

---

## Ready-to-run downloads

Choose a ZIP from **Assets**, extract it, and run `FloatNote.exe`. The Source code archives are optional developer downloads.

Choose **x64** for most Intel/AMD PCs, **ARM64** for Windows on ARM, or **x86** for a 32-bit environment.

2.0 adds liquid-glass edge refraction and reflections, a compact expandable control/settings panel, custom colors, animated note storage/restoration, and a remembered hide-or-exit choice. Native editing, local autosave, tray recovery, startup and keyboard shortcuts are retained.

Exit before upgrading, replace the EXE, and **keep your data folder**. Archives contain no personal data. Back up data first if you want to roll back.

Liquid glass targets supported Windows 11 environments; fallback applies when unavailable. Live glass may be absent from screenshots/third-party remote streams; select frosted glass when needed. New settings/material controls are currently Chinese. Binaries are unsigned. `SHA256SUMS.txt` contains archive checksums. CI compilation/native tests do not certify every GPU or remote environment.
