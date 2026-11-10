# 简单文本编辑器

Qt 应用程序开发实验二。使用 Qt 5.12.11、Qt Widgets、qmake 和 C++11，Windows 编译器为 MinGW 7.3.0 64 位。

## 构建与运行

在 Qt Creator 中打开 `TextEditor.pro`，选择 Desktop Qt 5.12.11 MinGW 64-bit Kit，构建后运行。界面文件 `mainwindow.ui`、`searchdialog.ui` 和 `aboutdialog.ui` 可在设计模式中编辑。主窗口的菜单和工具栏共享 Action，`CodeEditor` 在 `.ui` 中由 `QPlainTextEdit` 提升。

本机也可在 PowerShell 中运行：

```powershell
.\build.ps1
```

脚本构建程序与测试，使用 offscreen 平台执行测试并部署运行依赖，不弹出桌面窗口。完成后双击 `build\release\TextEditor.exe`。Qt 安装根目录不同时，使用 `-QtRoot` 指定包含 `5.12.11` 和 `Tools` 的目录。

## 操作

新建文档使用独立窗口。打开文件时复用空白未修改窗口，其他情况新开窗口。关闭有修改的文档时，可保存、放弃或取消；取消另存为也会取消关闭。

支持保存、另存为、撤销、重做、剪切、复制、粘贴、全选、字体和字号、字体颜色、背景色、自动换行、行号，以及工具栏和状态栏开关。状态栏显示字符数、逻辑行数及光标行列。字体和颜色仅影响当前窗口显示，纯文本文件不保存这些格式。

查找和替换是非模态对话框，可区分大小写和全词匹配。查找下一个到达末尾后从开头继续。单次替换操作针对当前匹配的选区，否则先定位下一处；全部替换从文档开头执行，结果显示替换数量，并可一次撤销。空查找词不可执行。

文件按 UTF-8 读取和保存，可读取 UTF-8 BOM。无效 UTF-8、未完整的 UTF-8 字节序列和包含 NUL 的二进制内容会被拒绝，原编辑内容保持不变。Windows 换行在编辑器中规范化，保存使用 LF。保存使用 `QSaveFile`，仅成功提交写入后更新路径和修改状态。定时自动保存为选做项，未实现。

## 测试

`tests/tests.pro` 为 Qt Test 工程。`build.ps1` 会执行文件读写、读写失败保护、关闭提示、编辑命令使能、视图开关、查找替换边界、行号滚动及多窗口独立性测试。结果位于 `build/tests/results.txt`。

测试中的事件过滤器仅处理 Qt 5.12 Windows `QMessageBox::showEvent` 在 offscreen 平台访问空的原生接口的问题，不改变正式程序的对话框行为。离屏插件可能输出不支持 raise、propagateSizeHints 的提示；这些提示不表示功能断言失败。

## 本地材料

实验说明书、执行方案、构建产物及 `local-materials/` 中的报告、截图、测试记录和导出日志均被忽略，不纳入 Git 提交。演示视频由用户自行录制。只有源码、界面、测试、构建脚本和本说明进入代码提交。
