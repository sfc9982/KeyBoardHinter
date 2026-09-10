#pragma once

// OSD 悬浮卡片窗口：无边框、不抢焦点，用于显示
//   1) 单个锁定键的图标 + 提示文字（按下 Caps/Num/Scroll Lock 时）
//   2) 三个锁定键状态的概览列表（托盘菜单"显示当前状态"）
//
// 绘制基准为 160x112 的逻辑坐标，实际窗口尺寸可经配置放大，
// 绘制时统一按比例缩放（见 OSDWindow.cpp::OnPaint）。

#include <wx/wx.h>

#include "LockState.h"

class OSDWindow : public wxFrame {
public:
	// 构造后窗口处于隐藏状态，由调用方决定何时显示。
	OSDWindow(int width, int height, int hideDelayMs, int posNum, int posDen);
	~OSDWindow() override;

	// 显示"某键切换后"的提示卡片（例如 "Caps Lock On" 配双 A 图标）。
	void ShowHint(const wxString &hintText, DWORD displayKey, bool displayOn);

	// 显示三个锁定键的当前状态概览（图标区域不绘制单个键）。
	void ShowOverview(const LockState &state);

	// 把当前内容画到任意上下文上，坐标按 160x112 基准缩放到 (width, height)。
	// OnPaint 只是它的薄封装，"渲染结果是否正确"因此可以离屏测试（见 OSDWindowTest.cpp）。
	void Render(wxGraphicsContext &gc, int width, int height) const;

private:
	void OnPaint(wxPaintEvent &);
	void OnHideTimer(wxTimerEvent &);

	// 刷新内容并按 hideDelayMs 后自动隐藏。
	void ShowNoActivate();

	// 用 SW_SHOWNOACTIVATE 显示，避免抢走当前活动窗口的键盘焦点。
	void RevealWindow();

	// 定位到屏幕 width/height 与 posNum/posDen 决定的纵向位置，水平居中。
	void PlaceOnScreen();

	// 概览模式：绘制三行状态文字而非单个键的图标。
	bool     m_overview = false;
	wxString m_hintText = "Caps Lock Off";
	DWORD    m_displayKey = VK_CAPITAL;
	bool     m_displayOn = false;
	LockState m_state;

	int m_hideDelayMs = 1000;
	int m_posNum = 7;
	int m_posDen = 8;
	wxTimer m_hideTimer;
};
