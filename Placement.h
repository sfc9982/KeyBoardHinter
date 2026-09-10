#pragma once

// 悬浮窗定位计算：纯整数运算，不依赖 wxWidgets / Win32，
// 因此可以脱离 GUI 做单元测试（见 ConfigTest.cpp）。

namespace placement {

	// 把数值 a 按分数 num/den 缩放并四舍五入：等价 round(a * num / den)。
	// 用整数加半再整除实现，避免引入 <cmath> 与浮点误差。
	constexpr int ScaleRound(int a, int num, int den)
	{
		if (den <= 0)
		{
			return a;
		}
		return (a * num + den / 2) / den;
	}

	// 悬浮窗中心的纵向坐标：屏幕高度 * num / den 处。
	// 默认 7/8 即屏幕靠下的八分之七处（略高于任务栏）。
	constexpr int CenterY(int screenHeight, int num, int den)
	{
		return ScaleRound(screenHeight, num, den);
	}

} // namespace placement
