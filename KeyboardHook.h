#pragma once

// 低级键盘钩子（WH_KEYBOARD_LL）封装：只把 Caps/Num/Scroll Lock 的"释放"事件
// 回调给上层，其余按键直接放行。
//
// 注意事项：
//   * 钩子回调运行在安装线程的消息循环内（本程序即主线程），因此回调里可以安全操作 UI；
//   * 只在 WM_KEYUP 上报，保证系统锁定状态已切换完成，并天然过滤按键自动重复；
//   * 钩子收不到发送给更高权限（管理员）窗口的按键，这是 Win32 的既定限制。

#include <functional>

#include <Windows.h>

class KeyboardHook {
public:
	using Handler = std::function<void(DWORD vkCode)>;

	KeyboardHook() = default;
	~KeyboardHook();

	KeyboardHook(const KeyboardHook &) = delete;
	KeyboardHook &operator=(const KeyboardHook &) = delete;

	// 安装钩子。失败返回 false（此时不会注册回调）。
	bool Install(Handler handler);

	// 卸载钩子；可重复调用。
	void Uninstall();

	// 该组合键是否由本模块负责上报。
	static bool IsLockKey(DWORD vkCode);

private:
	static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);

	HHOOK m_hook = nullptr;
	Handler m_handler;

	// 钩子回调是自由函数，拿不到 this，用静态指针指向当前实例。
	// 程序只会在主线程安装一个钩子，因此无需加锁。
	static KeyboardHook *s_instance;
};
