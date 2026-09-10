#pragma once

// 锁定键状态机：把"收到锁定键释放事件后应更新什么"的纯逻辑从 UI 中剥离，
// 使其不依赖 wxWidgets 与窗口状态，可脱离 Win32 UI 单独测试。
// 依赖仅限 <Windows.h> 的 VK_* 常量，其余均为标准库类型。
//
// 实现见 LockState.cpp。

#include <string>

#include <Windows.h>

class LockState {
public:
	struct Result {
		bool        changed    = false;
		DWORD       displayKey = 0;
		bool        displayOn  = false;
		std::string hintText;
	};

	LockState() = default;
	LockState(bool capslockOn, bool numlockOn, bool scrolllockOn = false)
		: m_capslockOn(capslockOn), m_numlockOn(numlockOn), m_scrolllockOn(scrolllockOn) {}

	// 上报某锁定键释放后的状态，isOn 为该键的真实开关状态（由调用方读取 GetKeyState）。
	// 状态无变化（例如按键自动重复）时返回 changed=false，调用方不应刷新或显示 OSD。
	Result Update(DWORD vkCode, bool isOn);

	bool IsCapsLockOn() const { return m_capslockOn; }
	bool IsNumLockOn() const { return m_numlockOn; }
	bool IsScrollLockOn() const { return m_scrolllockOn; }

private:
	bool m_capslockOn   = false;
	bool m_numlockOn    = false;
	bool m_scrolllockOn = false;
};
