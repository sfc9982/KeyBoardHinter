#include "LockState.h"

LockState::Result LockState::Update(DWORD vkCode, bool isOn)
{
	Result result;
	if (vkCode == VK_NUMLOCK && isOn != m_numlockOn)
	{
		// 与 Num Lock 相关的提示优先显示（保留旧行为）
		m_numlockOn = isOn;
		result.hintText = isOn ? "Num Lock On" : "Num Lock Off";
		result.changed = true;
	}
	else if (vkCode == VK_CAPITAL && isOn != m_capslockOn)
	{
		m_capslockOn = isOn;
		result.hintText = isOn ? "Caps Lock On" : "Caps Lock Off";
		result.changed = true;
	}
	else if (vkCode == VK_SCROLL && isOn != m_scrolllockOn)
	{
		m_scrolllockOn = isOn;
		result.hintText = isOn ? "Scroll Lock On" : "Scroll Lock Off";
		result.changed = true;
	}
	if (result.changed)
	{
		result.displayKey = vkCode;
		result.displayOn = isOn;
	}
	return result;
}
