#pragma once

// 锁定键状态机：把"收到锁定键释放事件后应更新什么"的纯逻辑从 MyFrame 中剥离，
// 使其不依赖 wxWidgets 与窗口状态，可脱离 Win32 UI 单独测试。
// 依赖仅限 <Windows.h> 的 VK_* 常量，其余均为标准库类型。

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
	LockState(bool capslockOn, bool numlockOn)
		: m_capslockOn(capslockOn), m_numlockOn(numlockOn) {}

	// 上报某锁定键释放后的状态，isOn 为该键的真实开关状态（由调用方读取 GetKeyState）。
	// 状态无变化（例如按键自动重复）时返回 changed=false，调用方不应刷新或显示 OSD。
	Result Update(DWORD vkCode, bool isOn) {
		Result result;
		if (vkCode == VK_NUMLOCK && isOn != m_numlockOn) {
			// 与 Num Lock 相关的提示优先显示（保留旧行为）
			m_numlockOn = isOn;
			result.hintText = isOn ? "Num Lock On" : "Num Lock Off";
			result.changed  = true;
		} else if (vkCode == VK_CAPITAL && isOn != m_capslockOn) {
			m_capslockOn = isOn;
			result.hintText = isOn ? "Caps Lock On" : "Caps Lock Off";
			result.changed  = true;
		}
		if (result.changed) {
			result.displayKey = vkCode;
			result.displayOn  = isOn;
		}
		return result;
	}

	bool IsCapsLockOn() const { return m_capslockOn; }
	bool IsNumLockOn() const { return m_numlockOn; }

private:
	bool m_capslockOn = false;
	bool m_numlockOn  = false;
};
