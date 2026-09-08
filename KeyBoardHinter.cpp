#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <memory>

#include <Windows.h>

namespace {

// 魔法数字集中定义，便于统一调整
constexpr int   kHideDelayMs    = 1000;
constexpr int   kWindowWidth    = 160;
constexpr int   kWindowHeight   = 112;
constexpr int   kVerticalPosNum = 7; // 悬浮窗垂直定位在屏幕 7/8 高度处
constexpr int   kVerticalPosDen = 8;

} // namespace

class MyFrame : public wxFrame {
public:
	MyFrame()
		: wxFrame(nullptr,
			wxID_ANY,
			"",
			wxDefaultPosition,
			wxSize(kWindowWidth, kWindowHeight),
			wxPOPUP_WINDOW | wxNO_BORDER | wxFRAME_NO_TASKBAR | wxFRAME_TOOL_WINDOW | wxSTAY_ON_TOP) {
		Hide();

		SetBackgroundColour(wxColour(244, 244, 244));
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetClientSize(FromDIP(wxSize(kWindowWidth, kWindowHeight)));
		Bind(wxEVT_PAINT, &MyFrame::PaintCard, this);
		PlaceOnScreen();

		// 快照启动时刻的真实锁定状态，避免把历史状态误当成"刚刚发生的变化"
		m_capslockOn = (::GetKeyState(VK_CAPITAL) & 1) != 0;
		m_numlockOn  = (::GetKeyState(VK_NUMLOCK) & 1) != 0;

		// 低级键盘钩子改为事件驱动：只在 Caps/Num 键真实按下时才被唤醒，
		// 替代原先 150ms 一次的轮询定时器——空闲时零开销、响应无延迟；
		// 顺带修复了轮询定时器每 150ms 重置隐藏倒计时、导致悬浮窗永不消失的问题。
		// 局限：钩子收不到发送给更高权限（管理员）窗口的按键，日常使用不受影响。
		s_instance = this;
		m_keyboardHook = ::SetWindowsHookExW(WH_KEYBOARD_LL,
			&MyFrame::LowLevelKeyboardProc,
			::GetModuleHandleW(nullptr),
			0);

		m_nap.Bind(wxEVT_TIMER, &MyFrame::TakeNap, this);
	}

	~MyFrame() override {
		if (m_keyboardHook != nullptr) {
			::UnhookWindowsHookEx(m_keyboardHook);
		}
		s_instance = nullptr;
	}

	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
		if (nCode == HC_ACTION && wParam == WM_KEYUP) {
			const auto &info = *reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
			if (info.vkCode == VK_CAPITAL || info.vkCode == VK_NUMLOCK) {
				// 钩子回调运行在安装线程（主线程）的 GetMessage 内，可直接操作 UI；
				// 只在 WM_KEYUP 处理，保证切换状态已生效，并天然过滤按键自动重复
				if (s_instance != nullptr) {
					s_instance->OnLockKeyToggled(info.vkCode);
				}
			}
		}
		return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
	}

	void OnLockKeyToggled(DWORD vkCode) {
		const bool isOn = (::GetKeyState(static_cast<int>(vkCode)) & 1) != 0;

		bool changed = false;
		if (vkCode == VK_NUMLOCK && isOn != m_numlockOn) {
			// 与 Num Lock 相关的提示优先显示（保留旧行为）
			m_numlockOn = isOn;
			m_hintText = isOn ? "Num Lock On" : "Num Lock Off";
			changed = true;
		} else if (vkCode == VK_CAPITAL && isOn != m_capslockOn) {
			m_capslockOn = isOn;
			m_hintText = isOn ? "Caps Lock On" : "Caps Lock Off";
			changed = true;
		}

		if (changed) {
			m_displayKey = vkCode;
			m_displayOn = isOn;
			Refresh(false);
			ShowNoActivate();
			// 只在状态真正变化时重启隐藏倒计时，1 秒后自动隐藏
			m_nap.Start(kHideDelayMs, wxTIMER_ONE_SHOT);
		}
	}

	void TakeNap(wxTimerEvent &event) {
#ifdef __WXMSW__
		::ShowWindow(GetHWND(), SW_HIDE);
#else
		Hide();
#endif
	}

private:
	void PaintCard(wxPaintEvent &) {
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(GetBackgroundColour()));
		dc.Clear();
		std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
		if (!gc) return;
		// 在逻辑尺寸上绘制，文字和图标随 Windows DPI 一起缩放。
		gc->Scale(static_cast<double>(GetClientSize().x) / kWindowWidth,
			static_cast<double>(GetClientSize().y) / kWindowHeight);
		const wxColour ink(20, 20, 20);
		gc->SetPen(wxPen(wxColour(224, 224, 224), 1));
		gc->SetBrush(*wxTRANSPARENT_BRUSH);
		gc->DrawRectangle(0.5, 0.5, kWindowWidth - 1, kWindowHeight - 1);

		if (m_displayKey == VK_CAPITAL) {
			// 双 A 使用矢量笔画，避免字体替换改变图标形状。
			gc->SetPen(wxPen(ink, 2.3));
			auto drawA = [&](double x, double y, double width, double height) {
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
		} else {
			// 数字锁：开启时锁梁闭合，关闭时右侧抬起。
			gc->SetPen(wxPen(ink, 2.3));
			auto shackle = gc->CreatePath();
			shackle.MoveToPoint(69, 33);
			shackle.AddLineToPoint(69, 29);
			if (m_displayOn) {
				shackle.AddCurveToPoint(69, 15, 91, 15, 91, 29);
				shackle.AddLineToPoint(91, 33);
			} else {
				shackle.AddCurveToPoint(69, 15, 84, 13, 90, 21);
			}
			gc->StrokePath(shackle);
			gc->DrawRectangle(66, 33, 28, 23);
			// 矢量数字 1，保持不同 DPI 下的笔画比例一致。
			gc->SetPen(wxPen(ink, 1.8));
			auto numeral = gc->CreatePath();
			numeral.MoveToPoint(77, 42);
			numeral.AddLineToPoint(80, 39);
			numeral.AddLineToPoint(80, 50);
			gc->StrokePath(numeral);
		}
		if (m_displayKey == VK_CAPITAL && !m_displayOn) {
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
	void ShowNoActivate() {
#ifdef __WXMSW__
		::ShowWindow(GetHWND(), SW_SHOWNOACTIVATE);
#else
		Show();
#endif
	}

	// 纯整数运算定位到屏幕 7/8 高度处（(h*7+4)/8 等价于四舍五入），
	// 避免 floor() 带来的隐式 <math.h> 依赖
	void PlaceOnScreen() {
		const wxSize screenSize = wxGetDisplaySize();
		const int    bottomY    = (screenSize.GetHeight() * kVerticalPosNum + kVerticalPosDen / 2) / kVerticalPosDen;
		CenterOnScreen(wxHORIZONTAL);
		Move(wxPoint(GetPosition().x, bottomY - GetSize().GetHeight() / 2));
	}

	wxString      m_hintText     = "Caps Lock Off";
	DWORD         m_displayKey   = VK_CAPITAL;
	bool          m_displayOn    = false;
	wxTimer       m_nap;
	bool          m_capslockOn   = false;
	bool          m_numlockOn    = false;
	HHOOK         m_keyboardHook = nullptr;

	static MyFrame *s_instance;
};

MyFrame *MyFrame::s_instance = nullptr;


class MyApp : public wxApp {
public:
	bool OnInit() override {
		// 先声明系统级 DPI 感知，否则 wxGetDisplaySize 返回逻辑像素，悬浮窗定位会偏移
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

		new MyFrame();
		return true;
	}
};

wxIMPLEMENT_APP(MyApp);
