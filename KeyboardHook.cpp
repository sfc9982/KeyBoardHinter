#include "KeyboardHook.h"

KeyboardHook *KeyboardHook::s_instance = nullptr;

KeyboardHook::~KeyboardHook()
{
	Uninstall();
}

bool KeyboardHook::IsLockKey(DWORD vkCode)
{
	return vkCode == VK_CAPITAL || vkCode == VK_NUMLOCK || vkCode == VK_SCROLL;
}

bool KeyboardHook::Install(Handler handler)
{
	if (m_hook != nullptr)
	{
		return true;
	}
	m_handler = std::move(handler);
	s_instance = this;

	m_hook = ::SetWindowsHookExW(WH_KEYBOARD_LL, &KeyboardHook::HookProc,
								 ::GetModuleHandleW(nullptr), 0);
	if (m_hook == nullptr)
	{
		m_handler = nullptr;
		s_instance = nullptr;
		return false;
	}
	return true;
}

void KeyboardHook::Uninstall()
{
	if (m_hook != nullptr)
	{
		::UnhookWindowsHookEx(m_hook);
		m_hook = nullptr;
	}
	if (s_instance == this)
	{
		s_instance = nullptr;
	}
	m_handler = nullptr;
}

LRESULT CALLBACK KeyboardHook::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && wParam == WM_KEYUP)
	{
		const auto &info = *reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
		if (IsLockKey(info.vkCode) && s_instance != nullptr && s_instance->m_handler)
		{
			s_instance->m_handler(info.vkCode);
		}
	}
	return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}
