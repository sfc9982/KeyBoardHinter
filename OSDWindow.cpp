#include "OSDWindow.h"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include "Placement.h"

namespace {

	// 绘制基准尺寸：所有绘制坐标都按此尺寸书写，再缩放到实际客户区。
	constexpr int kBaseWidth = 160;
	constexpr int kBaseHeight = 112;

	// 配色
	const wxColour kPanelBackground(244, 244, 244);
	const wxColour kInk(20, 20, 20);
	const wxColour kBorder(224, 224, 224);
	const wxColour kText(35, 35, 35);

	wxFont MakeFont(int pointSize)
	{
		return wxFont(wxFontInfo(wxSize(0, pointSize)).FaceName("Segoe UI"));
	}

} // namespace

OSDWindow::OSDWindow(int width, int height, int hideDelayMs, int posNum, int posDen)
	: wxFrame(nullptr,
			  wxID_ANY,
			  "KeyBoardHinter",
			  wxDefaultPosition,
			  wxSize(width, height),
			  wxPOPUP_WINDOW | wxNO_BORDER | wxFRAME_NO_TASKBAR | wxFRAME_TOOL_WINDOW | wxSTAY_ON_TOP),
	  m_hideDelayMs(hideDelayMs),
	  m_posNum(posNum),
	  m_posDen(posDen)
{
	Hide();

	SetBackgroundColour(kPanelBackground);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetClientSize(FromDIP(wxSize(width, height)));
	Bind(wxEVT_PAINT, &OSDWindow::OnPaint, this);
	m_hideTimer.Bind(wxEVT_TIMER, &OSDWindow::OnHideTimer, this);

	PlaceOnScreen();
}

OSDWindow::~OSDWindow() = default;

void OSDWindow::ShowHint(const wxString &hintText, DWORD displayKey, bool displayOn)
{
	m_overview = false;
	m_hintText = hintText;
	m_displayKey = displayKey;
	m_displayOn = displayOn;
	ShowNoActivate();
}

void OSDWindow::ShowOverview(const LockState &state)
{
	m_state = state;
	m_overview = true;
	ShowNoActivate();
}

void OSDWindow::ShowNoActivate()
{
	Refresh(false);
	RevealWindow();
	// 一次性定时器：重复启动即重新计时，无需先 Stop
	m_hideTimer.Start(m_hideDelayMs, wxTIMER_ONE_SHOT);
}

// 用 SHOWNOACTIVATE 显示 OSD，避免抢走当前活动窗口的键盘焦点
void OSDWindow::RevealWindow()
{
#ifdef __WXMSW__
	::ShowWindow(GetHWND(), SW_SHOWNOACTIVATE);
#else
	Show();
#endif
}

void OSDWindow::OnHideTimer(wxTimerEvent &)
{
#ifdef __WXMSW__
	::ShowWindow(GetHWND(), SW_HIDE);
#else
	Hide();
#endif
}

// 水平居中，纵向落在屏幕高度的 posNum/posDen 处（默认 7/8）
void OSDWindow::PlaceOnScreen()
{
	const wxSize screenSize = wxGetDisplaySize();
	const int centerY = placement::CenterY(screenSize.GetHeight(), m_posNum, m_posDen);
	CenterOnScreen(wxHORIZONTAL);
	Move(wxPoint(GetPosition().x, centerY - GetSize().GetHeight() / 2));
}

void OSDWindow::OnPaint(wxPaintEvent &)
{
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
	if (!gc)
	{
		return;
	}
	Render(*gc, GetClientSize().x, GetClientSize().y);
}

void OSDWindow::Render(wxGraphicsContext &gc, int width, int height) const
{
	if (width <= 0 || height <= 0)
	{
		return;
	}
	// 把 160x112 的基准坐标系映射到实际绘图区域
	gc.Scale(static_cast<double>(width) / kBaseWidth,
			 static_cast<double>(height) / kBaseHeight);
	gc.SetBrush(*wxTRANSPARENT_BRUSH);

	if (m_overview)
	{
		gc.SetFont(MakeFont(12), kText);
		const wxString capsText = m_state.IsCapsLockOn() ? wxString("On") : wxString("Off");
		const wxString numText = m_state.IsNumLockOn() ? wxString("On") : wxString("Off");
		const wxString scrollText = m_state.IsScrollLockOn() ? wxString("On") : wxString("Off");
		auto drawLine = [&](int y, const wxString &text)
		{
			double textWidth, textHeight;
			gc.GetTextExtent(text, &textWidth, &textHeight);
			gc.DrawText(text, (kBaseWidth - textWidth) / 2, y - textHeight / 2);
		};
		drawLine(36, wxString::Format("Caps Lock: %s", capsText));
		drawLine(56, wxString::Format("Num Lock: %s", numText));
		drawLine(76, wxString::Format("Scroll Lock: %s", scrollText));
		return;
	}

	gc.SetPen(wxPen(kBorder, 1));
	gc.DrawRectangle(0.5, 0.5, kBaseWidth - 1, kBaseHeight - 1);

	if (m_displayKey == VK_CAPITAL)
	{
		// 双 A 使用矢量笔画，避免字体替换改变图标形状。
		gc.SetPen(wxPen(kInk, 2.3));
		auto drawA = [&](double x, double y, double aWidth, double aHeight)
		{
			auto path = gc.CreatePath();
			path.MoveToPoint(x, y + aHeight);
			path.AddLineToPoint(x + aWidth / 2, y);
			path.AddLineToPoint(x + aWidth, y + aHeight);
			path.MoveToPoint(x + aWidth * 0.23, y + aHeight * 0.62);
			path.AddLineToPoint(x + aWidth * 0.77, y + aHeight * 0.62);
			gc.StrokePath(path);
		};
		drawA(61, 25, 18, 22);
		drawA(81, 29, 16, 18);
	}
	else
	{
		// Num Lock 与 Scroll Lock 共用锁体（开=闭合锁梁，关=打开锁梁），
		// 锁体内部符号区分键位：数字 1 表示 Num Lock，上下双箭头表示 Scroll Lock。
		gc.SetPen(wxPen(kInk, 2.3));
		auto shackle = gc.CreatePath();
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
		gc.StrokePath(shackle);
		gc.DrawRectangle(66, 33, 28, 23);

		gc.SetPen(wxPen(kInk, 1.8));
		auto mark = gc.CreatePath();
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
		gc.StrokePath(mark);
	}
	if (m_displayKey == VK_CAPITAL && !m_displayOn)
	{
		// 浅色描边让关闭斜线穿过图标时仍然清晰。
		gc.SetPen(wxPen(kPanelBackground, 6));
		gc.StrokeLine(61, 51, 99, 20);
		gc.SetPen(wxPen(kInk, 2));
		gc.StrokeLine(61, 51, 99, 20);
	}

	gc.SetFont(MakeFont(14), kText);
	double textWidth, textHeight;
	gc.GetTextExtent(m_hintText, &textWidth, &textHeight);
	gc.DrawText(m_hintText, (kBaseWidth - textWidth) / 2, 70 - textHeight / 2);
}
