// 程序入口：把各模块装配起来。
//
//   win32_ui      OSDWindow   悬浮卡片显示
//                 TrayIcon    托盘图标与菜单
//                 KeyboardHook 低级键盘钩子
//   logic         LockState   锁定键状态机（可脱离 UI 测试）
//                 Config      配置读写
//                 Placement   悬浮窗定位（可脱离 UI 测试）
//
// 本文件只负责：DPI 感知、单实例判定、装载配置、创建窗口/托盘/钩子，
// 以及把钩子事件与托盘动作接到 OSD 上。

#include <memory>

#include <wx/wx.h>

#include "AutoStart.h"
#include "Config.h"
#include "KeyboardHook.h"
#include "LockState.h"
#include "OSDWindow.h"
#include "TrayIcon.h"

namespace {

	// 单实例锁句柄：持有到进程退出，由 OS 随进程自动回收，崩溃也不会残留
	HANDLE g_singleInstanceMutex = nullptr;

	// 命名互斥量在"当前会话内同名即同物"，第二个实例创建时返回已有对象的句柄，
	// 以 ERROR_ALREADY_EXISTS 判定自己不是首个实例；Local\ 前缀使作用域限于登录会话
	bool TryAcquireSingleInstance()
	{
		g_singleInstanceMutex = ::CreateMutexW(nullptr, FALSE, L"Local\\KeyBoardHinter.SingleInstance");
		return g_singleInstanceMutex != nullptr && ::GetLastError() != ERROR_ALREADY_EXISTS;
	}

} // namespace

class KeyBoardHinterApp : public wxApp {
public:
	bool OnInit() override;
	int OnExit() override;

private:
	// 钩子回调：读取真实锁定状态，交给 LockState 判定是否需要提示。
	void OnLockKeyToggled(DWORD vkCode);

	// 托盘菜单"开机自启"的处理。
	void OnSetAutoStart(bool enable);

	std::unique_ptr<OSDWindow> m_osd;
	std::unique_ptr<TrayIcon> m_trayIcon;
	KeyboardHook m_keyboardHook;
	LockState m_state;
};

bool KeyBoardHinterApp::OnInit()
{
	// 先声明系统级 DPI 感知，否则 wxGetDisplaySize 返回逻辑像素，悬浮窗定位会偏移
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	// 已有实例在运行时给出可见反馈，避免用户误以为双击无反应；
	// 此时未创建任何窗口/钩子，直接退出不影响首个实例
	if (!TryAcquireSingleInstance())
	{
		::MessageBoxW(nullptr,
					  L"KeyBoardHinter 已在运行，请在系统托盘中查看。",
					  L"KeyBoardHinter",
					  MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
		return false;
	}

	Config config;
	config.Load();
	config.Sanitize();

	// 快照启动时刻的真实锁定状态，避免把历史状态误当成"刚刚发生的变化"
	m_state = LockState((::GetKeyState(VK_CAPITAL) & 1) != 0,
						(::GetKeyState(VK_NUMLOCK) & 1) != 0,
						(::GetKeyState(VK_SCROLL) & 1) != 0);

	m_osd = std::make_unique<OSDWindow>(config.windowWidth, config.windowHeight,
										config.hideDelayMs, config.verticalPosNum, config.verticalPosDen);

	m_trayIcon = std::make_unique<TrayIcon>(
		[]() { return autostart::IsEnabled(); },
		[this](bool enable) { OnSetAutoStart(enable); },
		[this]() { m_osd->ShowOverview(m_state); },
		[]() { wxTheApp->ExitMainLoop(); });

	if (!m_keyboardHook.Install([this](DWORD vkCode) { OnLockKeyToggled(vkCode); }))
	{
		wxMessageBox(wxString::FromUTF8(u8"键盘钩子安装失败，程序无法监听锁定键。"),
					 "KeyBoardHinter", wxOK | wxICON_ERROR);
		return false;
	}

	return true;
}

int KeyBoardHinterApp::OnExit()
{
	m_keyboardHook.Uninstall();
	m_trayIcon.reset();
	m_osd.reset();
	return wxApp::OnExit();
}

void KeyBoardHinterApp::OnLockKeyToggled(DWORD vkCode)
{
	const bool isOn = (::GetKeyState(static_cast<int>(vkCode)) & 1) != 0;
	// 状态是否变化、提示文本与显示内容由 LockState 独立判定（该逻辑可脱离 UI 测试）
	const LockState::Result result = m_state.Update(vkCode, isOn);
	if (result.changed)
	{
		m_osd->ShowHint(wxString::FromUTF8(result.hintText.c_str()), result.displayKey, result.displayOn);
	}
}

void KeyBoardHinterApp::OnSetAutoStart(bool enable)
{
	if (!autostart::SetEnabled(enable))
	{
		wxMessageBox(wxString::FromUTF8(u8"修改开机自启设置失败。"),
					 "KeyBoardHinter", wxOK | wxICON_WARNING);
	}
}

wxIMPLEMENT_APP(KeyBoardHinterApp);
