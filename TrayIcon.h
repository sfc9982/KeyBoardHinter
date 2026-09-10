#pragma once

// 系统托盘图标与右键菜单：开机自启、显示当前状态、退出。
// 菜单文案需要 UTF-8 字面量，统一在 TrayIcon.cpp 内用 wxString::FromUTF8 构造，
// 因此本头文件不出现非 ASCII 的字符串常量。

#include <functional>
#include <memory>
#include <string>

#include <wx/taskbar.h>
#include <wx/wx.h>

class TrayIcon : public wxTaskBarIcon {
public:
	TrayIcon(std::function<bool()> isAutoStartEnabled,
			 std::function<void(bool)> setAutoStartEnabled,
			 std::function<void()> onShowState,
			 std::function<void()> onQuit);
	~TrayIcon() override;

	wxMenu *CreatePopupMenu() override;

private:
	void OnLeftDown(wxTaskBarIconEvent &);
	void OnToggleAutoStart(wxCommandEvent &);
	void OnShowState(wxCommandEvent &);
	void OnQuit(wxCommandEvent &);

	// 程序化绘制托盘图标，避免引入 .ico 资源文件。
	static wxBitmap MakeBitmap();

	std::function<bool()> m_isAutoStartEnabled;
	std::function<void(bool)> m_setAutoStartEnabled;
	std::function<void()> m_onShowState;
	std::function<void()> m_onQuit;
};
