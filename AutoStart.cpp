#include "AutoStart.h"

#include <Windows.h>

namespace autostart {
	namespace {

		constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

	} // namespace

	std::wstring GetModulePath()
	{
		wchar_t buffer[MAX_PATH];
		const DWORD length = ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
		{
			return {};
		}
		return std::wstring(buffer, length);
	}

	bool IsEnabled()
	{
		HKEY key = nullptr;
		if (::RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &key) != ERROR_SUCCESS)
		{
			return false;
		}
		DWORD size = 0;
		const LSTATUS rc = ::RegQueryValueExW(key, kValueName, nullptr, nullptr, nullptr, &size);
		::RegCloseKey(key);
		return rc == ERROR_SUCCESS;
	}

	bool SetEnabled(bool enable)
	{
		const std::wstring exe = GetModulePath();
		if (exe.empty())
		{
			return false;
		}
		HKEY key = nullptr;
		if (::RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0,
							  KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
		{
			return false;
		}
		const LSTATUS rc = [&]() -> LSTATUS
		{
			if (!enable)
			{
				return ::RegDeleteValueW(key, kValueName);
			}
			// 路径可能含空格，必须加引号，否则 Windows 会按空格切分命令行
			const std::wstring command = L"\"" + exe + L"\"";
			return ::RegSetValueExW(key, kValueName, 0, REG_SZ,
									reinterpret_cast<const BYTE *>(command.c_str()),
									static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
		}();
		::RegCloseKey(key);
		return rc == ERROR_SUCCESS;
	}

} // namespace autostart
