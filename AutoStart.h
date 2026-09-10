#pragma once

// 开机自启：通过 HKCU\Software\Microsoft\Windows\CurrentVersion\Run 注册表项实现。
// 只依赖 Win32（advapi32 随 Windows 应用默认链接），不依赖 wxWidgets。

#include <string>

namespace autostart {

	// 注册表写入的值名，与 exe 同名便于用户识别。
	inline constexpr wchar_t kValueName[] = L"KeyBoardHinter";

	// 当前进程可执行文件的完整路径；失败返回空串。
	std::wstring GetModulePath();

	// 注册表中是否存在指向本程序的启动项。
	bool IsEnabled();

	// 写入（enable=true）或删除（enable=false）启动项；返回是否成功。
	bool SetEnabled(bool enable);

} // namespace autostart
