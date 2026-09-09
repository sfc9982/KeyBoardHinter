#include <wx/wx.h>
#include <wx/app.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>
#include <wx/taskbar.h>
#include <memory>
#include <string>

#include <Windows.h>

#include "Config.h"
#include "LockState.h"

namespace
{

	constexpr int kHideDelayMs = 1000;     // 默认隐藏延迟，可被配置文件覆盖
	constexpr int kWindowWidth = 160;      // 绘制基准宽度，物理窗口大小从配置读取
	constexpr int kWindowHeight = 112;     // 绘制基准高度
	constexpr int kVerticalPosNum = 7;     // 默认垂直位置分子（位置 = 高度 * num / den）
	constexpr int kVerticalPosDen = 8;     // 默认垂直位置分母

	enum
	{
		kAutoStartMenuId = wxID_HIGHEST + 1,
		kShowStateMenuId,
		kQuitMenuId,
	};

	constexpr wchar_t kAutoStartRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	constexpr wchar_t kAutoStartValueName[] = L"KeyBoardHinter";

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

	bool IsAutoStartEnabled()
	{
		HKEY key = nullptr;
		if (::RegOpenKeyExW(HKEY_CURRENT_USER, kAutoStartRunKey, 0, KEY_READ, &key) != ERROR_SUCCESS)
		{
			return false;
		}
		wchar_t value[MAX_PATH];
		DWORD size = sizeof(value);
		const LSTATUS rc = ::RegQueryValueExW(key, kAutoStartValueName, nullptr, nullptr,
											  reinterpret_cast<LPBYTE>(value), &size);
		::RegCloseKey(key);
		return rc == ERROR_SUCCESS;
	}

	bool SetAutoStartEnabled(bool enable)
	{
		const std::wstring exe = GetModulePath();
		if (exe.empty())
		{
			return false;
		}
		HKEY key = nullptr;
		if (::RegCreateKeyExW(HKEY_CURRENT_USER, kAutoStartRunKey, 0, nullptr, 0,
							  KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
		{
			return false;
		}
		const LSTATUS rc = [&]() -> LSTATUS
		{
			if (!enable)
			{
				return ::RegDeleteValueW(key, kAutoStartValueName);
			}
			const std::wstring command = L"\"" + exe + L"\"";
			return ::RegSetValueExW(key, kAutoStartValueName, 0, REG_SZ,
									reinterpret_cast<const BYTE *>(command.c_str()),
									static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
		}();
		::RegCloseKey(key);
		return rc == ERROR_SUCCESS;
	}

	// 程序化绘制托盘图标，避免引入 .ico 资源文件
	wxBitmap MakeTrayBitmap()
	{
		constexpr int size = 16;
		wxBitmap bmp(size, size, 32);
		{
			wxMemoryDC dc(bmp);
			dc.SetBackground(wxBrush(wxColour(0x2B, 0x6C, 0xB2)));
			dc.Clear();
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(wxBrush(wxColour(0x2B, 0x6C, 0xB2)));
			dc.DrawRoundedRectangle(0, 0, size, size, 3);
			dc.SetTextForeground(*wxWHITE);
			dc.SetFont(wxFont(wxSize(0, 11), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
			const wxSize textSize = dc.GetTextExtent("A");
			dc.DrawText("A", (size - textSize.x) / 2, (size - textSize.y) / 2);
		}
		return bmp;
	}

}

class MyFrame;

class TrayIcon : public wxTaskBarIcon
{
public:
	explicit TrayIcon(MyFrame *frame);
	wxMenu *CreatePopupMenu() override;

private:
	void OnLeftDown(wxTaskBarIconEvent &);
	void OnToggleAutoStart(wxCommandEvent &);
	void OnShowState(wxCommandEvent &);
	void OnQuit(wxCommandEvent &);

	MyFrame *m_frame;
};

class MyFrame : public wxFrame
{
public:
	MyFrame();
	~MyFrame() override;

	void ShowCurrentState();
	void QuitApp();

	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	void OnLockKeyToggled(DWORD vkCode);

private:
	void PaintCard(wxPaintEvent &);
	void TakeNap(wxTimerEvent &);
	void ShowNoActivate();
	void PlaceOnScreen();

	wxString m_hintText = "Caps Lock Off";
	DWORD m_displayKey = VK_CAPITAL;
	bool m_displayOn = false;
	bool m_overview = false;
	wxTimer m_nap;
	LockState m_state;
	HHOOK m_keyboardHook = nullptr;
	std::unique_ptr<TrayIcon> m_trayIcon;
	Config m_config;

	static MyFrame *s_instance;
};

TrayIcon::TrayIcon(MyFrame *frame)
	: m_frame(frame)
{
	SetIcon(wxBitmapBundle::FromBitmap(MakeTrayBitmap()), "KeyBoardHinter");
	Bind(wxEVT_TASKBAR_LEFT_DOWN, &TrayIcon::OnLeftDown, this);
}

wxMenu *TrayIcon::CreatePopupMenu()
{
	wxMenu *menu = new wxMenu();
	wxMenuItem *autoStartItem = menu->AppendCheckItem(kAutoStartMenuId, "开机自启");
	autoStartItem->Check(IsAutoStartEnabled());
	menu->Append(kShowStateMenuId, "显示当前状态");
	menu->AppendSeparator();
	menu->Append(kQuitMenuId, "退出");
	menu->Bind(wxEVT_MENU, &TrayIcon::OnToggleAutoStart, this, kAutoStartMenuId);
	menu->Bind(wxEVT_MENU, &TrayIcon::OnShowState, this, kShowStateMenuId);
	menu->Bind(wxEVT_MENU, &TrayIcon::OnQuit, this, kQuitMenuId);
	return menu;
}

void TrayIcon::OnLeftDown(wxTaskBarIconEvent &) { m_frame->ShowCurrentState(); }
void TrayIcon::OnToggleAutoStart(wxCommandEvent &event) { SetAutoStartEnabled(event.IsChecked()); }
void TrayIcon::OnShowState(wxCommandEvent &) { m_frame->ShowCurrentState(); }
void TrayIcon::OnQuit(wxCommandEvent &) { m_frame->QuitApp(); }

MyFrame::MyFrame()
	: wxFrame(nullptr,
			  wxID_ANY,
			  "",
			  wxDefaultPosition,
			  wxSize(kWindowWidth, kWindowHeight),
			  wxPOPUP_WINDOW | wxNO_BORDER | wxFRAME_NO_TASKBAR | wxFRAME_TOOL_WINDOW | wxSTAY_ON_TOP)
{
	m_config.Load();
	if (m_config.hideDelayMs <= 0) m_config.hideDelayMs = kHideDelayMs;
	if (m_config.windowWidth <= 0) m_config.windowWidth = kWindowWidth;
	if (m_config.windowHeight <= 0) m_config.windowHeight = kWindowHeight;
	if (m_config.verticalPosNum <= 0) m_config.verticalPosNum = kVerticalPosNum;
	if (m_config.verticalPosDen <= 0) m_config.verticalPosDen = kVerticalPosDen;

	Hide();

	SetBackgroundColour(wxColour(244, 244, 244));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetClientSize(FromDIP(wxSize(m_config.windowWidth, m_config.windowHeight)));
	Bind(wxEVT_PAINT, &MyFrame::PaintCard, this);
	PlaceOnScreen();

	// 快照启动时刻的真实锁定状态，避免把历史状态误当成"刚刚发生的变化"
	m_state = LockState((::GetKeyState(VK_CAPITAL) & 1) != 0,
						(::GetKeyState(VK_NUMLOCK) & 1) != 0,
						(::GetKeyState(VK_SCROLL) & 1) != 0);

	// 局限：钩子收不到发送给更高权限（管理员）窗口的按键，日常使用不受影响。
	s_instance = this;
	m_keyboardHook = ::SetWindowsHookExW(WH_KEYBOARD_LL,
										 &MyFrame::LowLevelKeyboardProc,
										 ::GetModuleHandleW(nullptr),
										 0);

	m_trayIcon = std::make_unique<TrayIcon>(this);

	m_nap.Bind(wxEVT_TIMER, &MyFrame::TakeNap, this);
}

MyFrame::~MyFrame()
{
	if (m_keyboardHook != nullptr)
	{
		::UnhookWindowsHookEx(m_keyboardHook);
	}
	s_instance = nullptr;
}

LRESULT CALLBACK MyFrame::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && wParam == WM_KEYUP)
	{
		const auto &info = *reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
		if (info.vkCode == VK_CAPITAL || info.vkCode == VK_NUMLOCK || info.vkCode == VK_SCROLL)
		{
			// 钩子回调运行在安装线程（主线程）的 GetMessage 内，可直接操作 UI；
			// 只在 WM_KEYUP 处理，保证切换状态已生效，并天然过滤按键自动重复
			if (s_instance != nullptr)
			{
				s_instance->OnLockKeyToggled(info.vkCode);
			}
		}
	}
	return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void MyFrame::OnLockKeyToggled(DWORD vkCode)
{
	const bool isOn = (::GetKeyState(static_cast<int>(vkCode)) & 1) != 0;
	// 状态是否变化、提示文本与显示内容由 LockState 独立判定（该逻辑可脱离 UI 测试）
	const LockState::Result result = m_state.Update(vkCode, isOn);

	if (result.changed)
	{
		m_overview = false;
		m_hintText = wxString(result.hintText);
		m_displayKey = result.displayKey;
		m_displayOn = result.displayOn;
		Refresh(false);
		ShowNoActivate();
		m_nap.Start(m_config.hideDelayMs, wxTIMER_ONE_SHOT);
	}
}

void MyFrame::ShowCurrentState()
{
	m_overview = true;
	Refresh(false);
	ShowNoActivate();
	m_nap.Start(m_config.hideDelayMs, wxTIMER_ONE_SHOT);
}

void MyFrame::QuitApp()
{
	wxTheApp->ExitMainLoop();
}

void MyFrame::TakeNap(wxTimerEvent &event)
{
#ifdef __WXMSW__
	::ShowWindow(GetHWND(), SW_HIDE);
#else
	Hide();
#endif
}

void MyFrame::PaintCard(wxPaintEvent &)
{
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
	if (!gc)
		return;
	gc->Scale(static_cast<double>(GetClientSize().x) / kWindowWidth,
			  static_cast<double>(GetClientSize().y) / kWindowHeight);

	if (m_overview)
	{
		gc->SetFont(wxFont(wxFontInfo(wxSize(0, 12)).FaceName("Segoe UI")), wxColour(35, 35, 35));
		const wxString capsText = m_state.IsCapsLockOn() ? wxString("On") : wxString("Off");
		const wxString numText = m_state.IsNumLockOn() ? wxString("On") : wxString("Off");
		const wxString scrollText = m_state.IsScrollLockOn() ? wxString("On") : wxString("Off");
		auto drawLine = [&](int y, const wxString &text)
		{
			double width, height;
			gc->GetTextExtent(text, &width, &height);
			gc->DrawText(text, (kWindowWidth - width) / 2, y - height / 2);
		};
		drawLine(36, wxString::Format("Caps Lock: %s", capsText));
		drawLine(56, wxString::Format("Num Lock: %s", numText));
		drawLine(76, wxString::Format("Scroll Lock: %s", scrollText));
		return;
	}

	const wxColour ink(20, 20, 20);
	gc->SetPen(wxPen(wxColour(224, 224, 224), 1));
	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->DrawRectangle(0.5, 0.5, kWindowWidth - 1, kWindowHeight - 1);

	if (m_displayKey == VK_CAPITAL)
	{
		// 双 A 使用矢量笔画，避免字体替换改变图标形状。
		gc->SetPen(wxPen(ink, 2.3));
		auto drawA = [&](double x, double y, double width, double height)
		{
			auto path = gc->CreatePath();
			path.MoveToPoint(x, y + height);
			path.AddLineToPoint(x + width / 2, y);
			path.AddLineToPoint(x + width, y + height);
			path.MoveToPoint(x + width * 0.23, y + height * 0.62);
			path.AddLineToPoint(x + width * 0.77, y + height * 0.62);
			gc->StrokePath(path);
		};
		drawA(61, 25, 18, 22);
		drawA(81, 29, 16, 18);
	}
	else
	{
		// Num Lock 与 Scroll Lock 共用锁体（开=闭合锁梁，关=打开锁梁），
		// 锁体内部符号区分键位：数字 1 表示 Num Lock，上下双箭头表示 Scroll Lock。
		gc->SetPen(wxPen(ink, 2.3));
		auto shackle = gc->CreatePath();
		shackle.MoveToPoint(69, 33);
		shackle.AddLineToPoint(69, 29);
		if (m_displayOn)
		{
			shackle.AddCurveToPoint(69, 15, 91, 15, 91, 29);
			shackle.AddLineToPoint(91, 33);
		}
		else
		{
			shackle.AddCurveToPoint(69, 15, 84, 13, 90, 21);
		}
		gc->StrokePath(shackle);
		gc->DrawRectangle(66, 33, 28, 23);

		gc->SetPen(wxPen(ink, 1.8));
		auto mark = gc->CreatePath();
		if (m_displayKey == VK_NUMLOCK)
		{
			mark.MoveToPoint(77, 42);
			mark.AddLineToPoint(80, 39);
			mark.AddLineToPoint(80, 50);
		}
		else
		{
			mark.MoveToPoint(80, 39);
			mark.AddLineToPoint(80, 50);
			mark.MoveToPoint(77, 42);
			mark.AddLineToPoint(80, 39);
			mark.AddLineToPoint(83, 42);
			mark.MoveToPoint(77, 47);
			mark.AddLineToPoint(80, 50);
			mark.AddLineToPoint(83, 47);
		}
		gc->StrokePath(mark);
	}
	if (m_displayKey == VK_CAPITAL && !m_displayOn)
	{
		// 浅色描边让关闭斜线穿过图标时仍然清晰。
		gc->SetPen(wxPen(GetBackgroundColour(), 6));
		gc->StrokeLine(61, 51, 99, 20);
		gc->SetPen(wxPen(ink, 2));
		gc->StrokeLine(61, 51, 99, 20);
	}
	gc->SetFont(wxFont(wxFontInfo(wxSize(0, 14)).FaceName("Segoe UI")), wxColour(35, 35, 35));
	double width, height;
	gc->GetTextExtent(m_hintText, &width, &height);
	gc->DrawText(m_hintText, (kWindowWidth - width) / 2, 70 - height / 2);
}

// 用 SHOWNOACTIVATE 显示 OSD，避免抢走当前活动窗口的键盘焦点
void MyFrame::ShowNoActivate()
{
#ifdef __WXMSW__
	::ShowWindow(GetHWND(), SW_SHOWNOACTIVATE);
#else
	Show();
#endif
}

// 纯整数运算定位到屏幕 7/8 高度处（(h*7+4)/8 等价于四舍五入），
// 避免 floor() 带来的隐式 <math.h> 依赖
void MyFrame::PlaceOnScreen()
{
	const wxSize screenSize = wxGetDisplaySize();
	const int posNum = m_config.verticalPosNum;
	const int posDen = m_config.verticalPosDen;
	const int bottomY = (screenSize.GetHeight() * posNum + posDen / 2) / posDen;
	CenterOnScreen(wxHORIZONTAL);
	Move(wxPoint(GetPosition().x, bottomY - GetSize().GetHeight() / 2));
}

MyFrame *MyFrame::s_instance = nullptr;

class MyApp : public wxApp
{
public:
	bool OnInit() override
	{
		// 先声明系统级 DPI 感知，否则 wxGetDisplaySize 返回逻辑像素，悬浮窗定位会偏移
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

		new MyFrame();
		return true;
	}
};

wxIMPLEMENT_APP(MyApp);
