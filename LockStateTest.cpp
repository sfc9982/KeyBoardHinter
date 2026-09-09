// LockState 状态机的单元测试：不依赖 wxWidgets 与 Win32 UI，可独立构建运行。
// 项目未引入第三方测试框架，这里使用自带轻量断言，保持零额外依赖；
// 由 CMake 注册到 CTest（ctest --test-dir build -C Debug）执行。

#include "LockState.h"

#include <cstdio>

#include <Windows.h>

namespace {

int g_failures = 0;

void Check(bool condition, const char *file, int line, const char *expr) {
	if (!condition) {
		std::printf("FAIL %s:%d: %s\n", file, line, expr);
		++g_failures;
	}
}

} // namespace

#define CHECK(expr) Check((expr), __FILE__, __LINE__, #expr)

namespace {

void TestInitialDefaults() {
	LockState state;
	CHECK(!state.IsCapsLockOn());
	CHECK(!state.IsNumLockOn());
}

void TestCapsLockToggle() {
	LockState state;

	const LockState::Result on = state.Update(VK_CAPITAL, true);
	CHECK(on.changed);
	CHECK(on.displayKey == VK_CAPITAL);
	CHECK(on.displayOn);
	CHECK(on.hintText == "Caps Lock On");
	CHECK(state.IsCapsLockOn());
	CHECK(!state.IsNumLockOn());

	// 状态未变化（自动重复不会产生 WM_KEYUP，但即使上报也不应产生提示）
	const LockState::Result repeat = state.Update(VK_CAPITAL, true);
	CHECK(!repeat.changed);

	const LockState::Result off = state.Update(VK_CAPITAL, false);
	CHECK(off.changed);
	CHECK(off.displayKey == VK_CAPITAL);
	CHECK(!off.displayOn);
	CHECK(off.hintText == "Caps Lock Off");
	CHECK(!state.IsCapsLockOn());
}

void TestNumLockToggle() {
	LockState state;

	const LockState::Result on = state.Update(VK_NUMLOCK, true);
	CHECK(on.changed);
	CHECK(on.displayKey == VK_NUMLOCK);
	CHECK(on.displayOn);
	CHECK(on.hintText == "Num Lock On");
	CHECK(state.IsNumLockOn());
	CHECK(!state.IsCapsLockOn());

	const LockState::Result off = state.Update(VK_NUMLOCK, false);
	CHECK(off.changed);
	CHECK(off.displayKey == VK_NUMLOCK);
	CHECK(!off.displayOn);
	CHECK(off.hintText == "Num Lock Off");
	CHECK(!state.IsNumLockOn());
}

void TestNumLockPriority() {
	// 两键状态独立跟踪，且 Num Lock 事件优先于 Caps Lock 显示（保留旧行为）
	LockState state;
	state.Update(VK_CAPITAL, true);
	const LockState::Result num = state.Update(VK_NUMLOCK, true);
	CHECK(num.changed);
	CHECK(num.displayKey == VK_NUMLOCK);
	CHECK(num.hintText == "Num Lock On");
	CHECK(state.IsCapsLockOn());
	CHECK(state.IsNumLockOn());

	// Num Lock 事件不会触发 Caps Lock 的提示
	const LockState::Result again = state.Update(VK_NUMLOCK, true);
	CHECK(!again.changed);
}

void TestInitialSnapshot() {
	// 启动快照真实状态：Caps 已开启时收到"开启"事件不应提示，收到"关闭"才提示
	LockState state(true, false);
	CHECK(state.IsCapsLockOn());
	CHECK(!state.IsNumLockOn());

	const LockState::Result noop = state.Update(VK_CAPITAL, true);
	CHECK(!noop.changed);

	const LockState::Result off = state.Update(VK_CAPITAL, false);
	CHECK(off.changed);
	CHECK(off.hintText == "Caps Lock Off");
	CHECK(!state.IsCapsLockOn());
}

} // namespace

int main() {
	TestInitialDefaults();
	TestCapsLockToggle();
	TestNumLockToggle();
	TestNumLockPriority();
	TestInitialSnapshot();

	if (g_failures == 0) {
		std::printf("All tests passed.\n");
		return 0;
	}
	std::printf("%d check(s) failed.\n", g_failures);
	return 1;
}
