# FloatNote 2.0

[English](README.en.md) · 简体中文

原生 Windows 桌面便签，支持液态玻璃、毛玻璃和纯色外观。文字保留原生编辑、选择、输入法和撤销，便签与设置自动保存到本地。

## 直接下载使用

**[下载最新版便携包](https://github.com/Fryingpan-Jason/FloatNote/releases/latest)**，解压后双击 `FloatNote.exe`，无需编译源码、安装开发工具或额外运行库。

| 下载文件 | 适用设备 |
| --- | --- |
| `FloatNote-2.0.0-windows-x64.zip` | 大多数 Intel / AMD 64 位电脑，优先选择 |
| `FloatNote-2.0.0-windows-arm64.zip` | Windows on ARM 设备，如 Snapdragon X |
| `FloatNote-2.0.0-windows-x86.zip` | 32 位环境或需要 32 位程序时 |

不要下载页面底部的 **Source code** 来当作应用；那是给开发者的源码。将程序解压到可写目录，不建议放进 `Program Files`。程序未签名，Windows 可能显示 SmartScreen 提示；发布页提供 SHA-256 校验文件。

## 2.0 功能

- 液态玻璃：连续边缘折射、随背景变化的反光、柔化与可调色散；可切换系统毛玻璃或纯色。
- 顶部操作区按需展开，提供设置和关闭；移开后收回，不占用整个便签的悬浮区域。
- 向下压缩窗口高度可将便签收纳为带状态点的小条，点击即可恢复；收放和回弹保持连续。
- 置顶、鼠标穿透、背景/文字颜色预设、自定义颜色、字号和模糊设置。
- 首次关闭可选择隐藏到托盘或退出，并记住选择。
- 单实例、托盘、开机启动和全局恢复快捷键。

## 操作与升级

- 将鼠标移到顶部提示附近，打开设置；从右下角拖动调整大小。
- `Ctrl+Alt+E`：显示并恢复编辑；`Ctrl+Alt+H`：显示/隐藏；`Ctrl+Alt+P`：切换鼠标穿透。
- 双击托盘图标或再次启动程序也能找回便签。
- `Ctrl+滚轮`、`Ctrl++`、`Ctrl+-` 调整字号。
- 从 1.x 升级：先退出旧程序，用新版 `FloatNote.exe` 替换旧文件，**保留原目录中的 `data` 文件夹**。建议升级前复制一份 `data` 备份。

发布包不附带任何便签或个人设置；第一次运行会创建 `data/note.txt`、`settings.ini`、`material.ini` 和 `experience.ini`。程序不联网，不使用账户或遥测。液态玻璃通过系统 API 在本机 GPU 上处理屏幕背景，不上传屏幕内容。

## 平台说明

液态玻璃主要面向支持 Windows Graphics Capture / Composition 的 Windows 11。高对比度、节能、远程桌面或不支持的系统会降级；具体范围见[兼容性说明](docs/COMPATIBILITY.zh-CN.md)。实时液态玻璃的便签可能不出现在截图或第三方远程画面中，需要时改用毛玻璃；顶部操作区独立绘制，提供恢复入口。

2.0 新增设置面板、关闭确认和材质参数目前为中文；原有托盘与基础界面保留中英文选项。此版本不承诺新面板已完整英文翻译。

## 开发

需要 Visual Studio C++ 工具链和 Windows SDK。默认构建就是 2.0 桌面版：

```powershell
.\build.ps1 -Architecture x64 -OutputDirectory build\x64
.\build.ps1 -Test -OutputDirectory build\tests
.\scripts\package.ps1 -Architecture x64
```

详见[开发说明](docs/DEVELOPMENT.md)。基于 MIT 许可发布，材质算法适配的许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
