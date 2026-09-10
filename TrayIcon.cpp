#include "TrayIcon.h"

#include <wx/dcmemory.h>

namespace {

	// 菜单项 ID 只在本文件内使用，避免与其他模块的 ID 冲突。
	enum
	{
		kAutoStartMenuId = wxID_HIGHEST + 100,
		kShowStateMenuId,
		kQuitMenuId,
	};

	// 菜单文案为 UTF-8 字面量，显式转换以免受源码/执行字符集设置影响。
	wxString Utf8(const char *text)
	{
		return wxString::FromUTF8(text);
	}

} // namespace

TrayIcon::TrayIcon(std::function<bool()> isAutoStartEnabled,
				   std::function<void(bool)> setAutoStartEnabled,
				   std::function<void()> onShowState,
				   std::function<void()> onQuit)
	: m_isAutoStartEnabled(std::move(isAutoStartEnabled)),
	  m_setAutoStartEnabled(std::move(setAutoStartEnabled)),
	  m_onShowState(std::move(onShowState)),
	  m_onQuit(std::move(onQuit))
{
	SetIcon(wxBitmapBundle::FromBitmap(MakeBitmap()), "KeyBoardHinter");
	Bind(wxEVT_TASKBAR_LEFT_DOWN, &TrayIcon::OnLeftDown, this);
}

TrayIcon::~TrayIcon() = default;

wxBitmap TrayIcon::MakeBitmap()
{
	constexpr int size = 16;
	const wxColour background(0x2B, 0x6C, 0xB2);
	wxBitmap bmp(size, size, 32);
	{
		wxMemoryDC dc(bmp);
		dc.SetBackground(wxBrush(background));
		dc.Clear();
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(wxBrush(background));
		dc.DrawRoundedRectangle(0, 0, size, size, 3);
		dc.SetTextForeground(*wxWHITE);
		dc.SetFont(wxFont(wxSize(0, 11), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		const wxSize textSize = dc.GetTextExtent("A");
		dc.DrawText("A", (size - textSize.x) / 2, (size - textSize.y) / 2);
	}
	return bmp;
}

wxMenu *TrayIcon::CreatePopupMenu()
{
	wxMenu *menu = new wxMenu();
	wxMenuItem *autoStartItem = menu->AppendCheckItem(kAutoStartMenuId, Utf8(u8"开机自启"));
	autoStartItem->Check(m_isAutoStartEnabled ? m_isAutoStartEnabled() : false);
	menu->Append(kShowStateMenuId, Utf8(u8"显示当前状态"));
	menu->AppendSeparator();
	menu->Append(kQuitMenuId, Utf8(u8"退出"));
	menu->Bind(wxEVT_MENU, &TrayIcon::OnToggleAutoStart, this, kAutoStartMenuId);
	menu->Bind(wxEVT_MENU, &TrayIcon::OnShowState, this, kShowStateMenuId);
	menu->Bind(wxEVT_MENU, &TrayIcon::OnQuit, this, kQuitMenuId);
	return menu;
}

void TrayIcon::OnLeftDown(wxTaskBarIconEvent &)
{
	if (m_onShowState)
	{
		m_onShowState();
	}
}

void TrayIcon::OnToggleAutoStart(wxCommandEvent &event)
{
	if (m_setAutoStartEnabled)
	{
		m_setAutoStartEnabled(event.IsChecked());
	}
}

void TrayIcon::OnShowState(wxCommandEvent &)
{
	if (m_onShowState)
	{
		m_onShowState();
	}
}

void TrayIcon::OnQuit(wxCommandEvent &)
{
	if (m_onQuit)
	{
		m_onQuit();
	}
}
