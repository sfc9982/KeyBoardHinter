// Placement 单元测试：验证悬浮窗定位的整数运算（不依赖 wxWidgets / Win32 UI）。
// 由 CTest 注册执行。

#include "Placement.h"

#include <cstdio>

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

} // namespace

#define CHECK(expr) Check((expr), __LINE__, #expr)

namespace {

	void TestScaleRound()
	{
		CHECK(placement::ScaleRound(1080, 7, 8) == 945);
		CHECK(placement::ScaleRound(800, 7, 8) == 700);
		CHECK(placement::ScaleRound(0, 7, 8) == 0);
		// 四舍五入而非截断：1080 * 1 / 3 = 360，1081 * 1 / 3 = 360.33 -> 360
		CHECK(placement::ScaleRound(1081, 1, 3) == 360);
		// 0.5 以上进位：1082 / 3 = 360.67 -> 361
		CHECK(placement::ScaleRound(1082, 1, 3) == 361);
		// 分母非法时按原值返回，绝不产生除零
		CHECK(placement::ScaleRound(500, 7, 0) == 500);
		CHECK(placement::ScaleRound(500, 7, -8) == 500);
	}

	void TestCenterYDefaults()
	{
		// 默认位置：屏幕 7/8 高度处
		CHECK(placement::CenterY(1080, 7, 8) == 945);
		CHECK(placement::CenterY(1440, 7, 8) == 1260);
		CHECK(placement::CenterY(2160, 7, 8) == 1890);
		// 1/2 即屏幕正中
		CHECK(placement::CenterY(1080, 1, 2) == 540);
	}

	void TestCenterYStaysInsideScreen()
	{
		CHECK(placement::CenterY(768, 7, 8) == 672);
		CHECK(placement::CenterY(768, 7, 8) < 768);
	}

} // namespace

int main()
{
	TestScaleRound();
	TestCenterYDefaults();
	TestCenterYStaysInsideScreen();

	if (g_failures == 0)
	{
		std::printf("All Placement tests passed.\n");
		return 0;
	}
	std::printf("%d check(s) failed.\n", g_failures);
	return 1;
}
