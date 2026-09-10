// OSDWindow 离屏渲染测试：不显示任何窗口，直接把卡片画到内存位图上，
// 再按像素判断"该画的东西有没有画出来"。这样绘制逻辑的回归可以被自动捕获，
// 而不再依赖人眼看一眼。
//
// 需要 wxWidgets（GraphicsContext 依赖 GUI 库），因此这里用 wxEntryStart/Cleanup
// 手动完成 GUI 初始化；由 CTest 注册执行。

#include <wx/wx.h>
#include <wx/graphics.h>

#include <cstdio>
#include <memory>

#include "LockState.h"
#include "OSDWindow.h"

namespace {

	int g_failures = 0;

	void Check(bool condition, int line, const char *expr)
	{
		if (!condition)
		{
			std::printf("FAIL line %d: %s\n", line, expr);
			++g_failures;
		}
	}

	// 基准绘制区域 160x112 放大 2 倍，便于用整数像素精确定位。
	constexpr int kWidth = 320;
	constexpr int kHeight = 224;

	// 把基准坐标 (x, y) 映射到测试位图坐标。
	wxPoint At(int x, int y)
	{
		return wxPoint(x * 2 + 1, y * 2 + 1);
	}

	const wxColour kInk(20, 20, 20);
	const wxColour kBorder(224, 224, 224);

	// 判断某像素与目标颜色是否接近（允许抗锯齿带来的少量偏色）。
	bool IsNear(const wxImage &image, const wxPoint &point, const wxColour &color, int tolerance = 90)
	{
		if (point.x < 0 || point.y < 0 ||
			point.x >= image.GetWidth() || point.y >= image.GetHeight())
		{
			return false;
		}
		const int distance = std::abs(static_cast<int>(image.GetRed(point.x, point.y)) - color.Red()) +
							 std::abs(static_cast<int>(image.GetGreen(point.x, point.y)) - color.Green()) +
							 std::abs(static_cast<int>(image.GetBlue(point.x, point.y)) - color.Blue());
		return distance <= tolerance;
	}

	bool IsInk(const wxImage &image, const wxPoint &point)
	{
		return IsNear(image, point, kInk);
	}

	// 统计整张图上"深色像素"的数量，用于判断图标/文字是否真的画了上去。
	int CountDarkPixels(const wxImage &image)
	{
		int count = 0;
		for (int y = 0; y < image.GetHeight(); ++y)
		{
			for (int x = 0; x < image.GetWidth(); ++x)
			{
				if (IsInk(image, wxPoint(x, y)))
				{
					++count;
				}
			}
		}
		return count;
	}

	wxImage RenderToImage(OSDWindow &window)
	{
		wxBitmap bitmap(kWidth, kHeight, 32);
		{
			wxMemoryDC dc(bitmap);
			dc.SetBackground(*wxWHITE_BRUSH);
			dc.Clear();
			std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
			if (gc)
			{
				window.Render(*gc, kWidth, kHeight);
			}
		}
		return bitmap.ConvertToImage();
	}

	// 单键提示：Caps Lock 开/关都应画出双 A 图标，关闭时多一条斜线。
	void TestCapsLockHint()
	{
		OSDWindow window(160, 112, 1000, 7, 8);

		window.ShowHint("Caps Lock Off", VK_CAPITAL, false);
		const wxImage off = RenderToImage(window);
		Check(CountDarkPixels(off) > 500, __LINE__, "Caps Lock Off 卡片应有可见内容");

		window.ShowHint("Caps Lock On", VK_CAPITAL, true);
		const wxImage on = RenderToImage(window);
		Check(CountDarkPixels(on) > 500, __LINE__, "Caps Lock On 卡片应有可见内容");

		// "关闭"状态的斜线经过 (80, 35) 附近，开启状态该处应为空白
		Check(IsInk(off, At(80, 35)), __LINE__, "Caps Lock Off 应在图标上画出斜线");
		Check(!IsInk(on, At(80, 35)), __LINE__, "Caps Lock On 不应有斜线");
	}

	// Num Lock / Scroll Lock 的区分点在锁体内部符号与锁梁开合。
	void TestLockBodyHints()
	{
		OSDWindow window(160, 112, 1000, 7, 8);

		window.ShowHint("Num Lock On", VK_NUMLOCK, true);
		const wxImage numOn = RenderToImage(window);
		// 锁体矩形 (66,33)-(94,56)：取底边中点验证锁体被画出
		Check(IsInk(numOn, At(80, 55)), __LINE__, "Num Lock 应画出锁体底边");
		// 锁梁开启时不会经过 (80, 16)，闭合时会
		Check(!IsInk(numOn, At(80, 16)), __LINE__, "Num Lock On 的锁梁应为打开状态");

		window.ShowHint("Num Lock Off", VK_NUMLOCK, false);
		const wxImage numOff = RenderToImage(window);
		Check(IsInk(numOff, At(80, 16)), __LINE__, "Num Lock Off 的锁梁应为闭合状态");
		// 与 Caps Lock 的图标不同：不应出现双 A 所在位置的笔画
		Check(CountDarkPixels(numOff) > 400, __LINE__, "Num Lock Off 卡片应有可见内容");

		window.ShowHint("Scroll Lock On", VK_SCROLL, true);
		const wxImage scrollOn = RenderToImage(window);
		Check(IsInk(scrollOn, At(80, 55)), __LINE__, "Scroll Lock 应画出锁体底边");
		Check(!IsInk(scrollOn, At(80, 16)), __LINE__, "Scroll Lock On 的锁梁应为打开状态");
	}

	// 概览模式：三行文字，图标区域应为空白。
	void TestOverview()
	{
		OSDWindow window(160, 112, 1000, 7, 8);
		LockState state(true, false, true);
		window.ShowOverview(state);
		const wxImage overview = RenderToImage(window);

		Check(CountDarkPixels(overview) > 400, __LINE__, "概览卡片应有三行文字");
		// 概览模式只画文字、不画单键图标，因此没有边框。用卡片上边界验证"无边框"。
		Check(!IsNear(overview, At(80, 1), kBorder), __LINE__, "概览模式不应绘制边框");

		// 对照：单键提示卡片应画出边框与图标。
		OSDWindow single(160, 112, 1000, 7, 8);
		single.ShowHint("Caps Lock On", VK_CAPITAL, true);
		const wxImage singleCard = RenderToImage(single);
		Check(IsNear(singleCard, At(80, 0), kBorder), __LINE__, "单键卡片应绘制上边框");
		// 图标面积远大于文字，用深色像素总数验证确实画出了双 A 图标（实测 ~1154）
		Check(CountDarkPixels(singleCard) > 500, __LINE__, "单键卡片应绘制图标笔画");

		// 全部开启/全部关闭都应能正常渲染（文字内容变化，但都有内容）
		LockState allOn(true, true, true);
		window.ShowOverview(allOn);
		Check(CountDarkPixels(RenderToImage(window)) > 400, __LINE__, "全开概览应有内容");
	}

	// 窗口尺寸放大后，绘制应等比缩放而不是留在左上角。
	void TestScaleFollowsTargetSize()
	{
		OSDWindow window(160, 112, 1000, 7, 8);
		window.ShowHint("Caps Lock On", VK_CAPITAL, true);
		const wxImage large = RenderToImage(window);

		// 注意：不能把变量命名为 small —— 它与编译环境/头文件中的标识符冲突，
		// 会让 MSVC 把 wxImage 声明解析成函数声明。这里用 smallCard。
		wxBitmap smallBitmap(160, 112, 32);
		{
			wxMemoryDC dc(smallBitmap);
			dc.SetBackground(*wxWHITE_BRUSH);
			dc.Clear();
			std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
			if (gc)
			{
				window.Render(*gc, 160, 112);
			}
		}
		const wxImage smallCard = smallBitmap.ConvertToImage();

		const int largeCount = CountDarkPixels(large);
		const int smallCount = CountDarkPixels(smallCard);

		// 2 倍尺寸下深色像素数量应显著多于 1 倍（线条按面积缩放）
		Check(largeCount > smallCount * 3 / 2,
			  __LINE__, "放大绘制时内容应同步缩放");
		// 1 倍尺寸下，右下角的文字区域也必须被画到（说明没有全部挤在左上角）。
		// 179 是实测值（边框为浅色不计入深色计数），阈值从宽以免误报。
		Check(smallCount > 120, __LINE__, "1 倍尺寸也应完整绘制");
	}

} // namespace

int main(int argc, char **argv)
{
	// 只初始化 wxWidgets，不进入消息循环、不显示窗口
	wxApp::SetInstance(new wxApp());
	if (!wxEntryStart(argc, argv))
	{
		std::printf("FAIL: wxEntryStart 失败，无法初始化 GUI（需要桌面会话）\n");
		return 1;
	}
	{
		wxTheApp->OnInit();

		TestCapsLockHint();
		TestLockBodyHints();
		TestOverview();
		TestScaleFollowsTargetSize();

		wxTheApp->OnExit();
	}
	wxEntryCleanup();

	if (g_failures == 0)
	{
		std::printf("All OSDWindow tests passed.\n");
		return 0;
	}
	std::printf("%d check(s) failed.\n", g_failures);
	return 1;
}
