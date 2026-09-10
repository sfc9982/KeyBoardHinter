// Config 单元测试：验证默认值、读写往返、注释/非法值处理与配置路径。
// 不依赖 wxWidgets 与 Win32 UI，可独立构建运行，由 CTest 注册执行。
//
// 用法：ConfigTest [临时配置文件路径]
// 路径缺省为当前目录下的 KeyBoardHinter.test.ini；测试结束会删除该文件，
// 因此不会触碰 %APPDATA% 下的真实配置。

#include "Config.h"

#include <cstdio>
#include <string>

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

	std::wstring g_path = L"KeyBoardHinter.test.ini";

	void WriteFile(const std::wstring &path, const std::wstring &content)
	{
		std::wofstream file(path.c_str(), std::ios::trunc);
		file << content;
	}

	std::wstring ReadFile(const std::wstring &path)
	{
		std::wifstream file(path.c_str());
		std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
		return content;
	}

	bool FileExists(const std::wstring &path)
	{
		return ::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	void RemoveFile(const std::wstring &path)
	{
		::DeleteFileW(path.c_str());
	}

} // namespace

#define CHECK(expr) Check((expr), __LINE__, #expr)

namespace {

	void TestDefaults()
	{
		Config config;
		CHECK(config.hideDelayMs == 1000);
		CHECK(config.windowWidth == 160);
		CHECK(config.windowHeight == 112);
		CHECK(config.verticalPosNum == 7);
		CHECK(config.verticalPosDen == 8);
	}

	void TestSanitizeRejectsNonPositiveValues()
	{
		Config config;
		config.hideDelayMs = 0;
		config.windowWidth = -5;
		config.windowHeight = 0;
		config.verticalPosNum = -1;
		config.verticalPosDen = 0;
		config.Sanitize();

		CHECK(config.hideDelayMs == 1000);
		CHECK(config.windowWidth == 160);
		CHECK(config.windowHeight == 112);
		CHECK(config.verticalPosNum == 7);
		CHECK(config.verticalPosDen == 8);
	}

	void TestSaveCreatesFileWithDefaultsWhenMissing()
	{
		RemoveFile(g_path);
		Config config;
		config.Load(g_path); // 文件不存在时应写出默认配置
		CHECK(FileExists(g_path));

		const std::wstring content = ReadFile(g_path);
		CHECK(content.find(L"hideDelayMs=1000") != std::wstring::npos);
		CHECK(content.find(L"windowWidth=160") != std::wstring::npos);
		CHECK(content.find(L"windowHeight=112") != std::wstring::npos);
		CHECK(content.find(L"verticalPosNum=7") != std::wstring::npos);
		CHECK(content.find(L"verticalPosDen=8") != std::wstring::npos);
		// 文件应带注释，便于用户直接编辑
		CHECK(content.find(L";") != std::wstring::npos);
		// 回归防护：wofstream 遇到无法转成本地代码页的字符会置 badbit 并静默丢弃
		// 后续写入，表现为文件里只剩注释行。这里确保全部 5 个键都真正落盘。
		CHECK(content.find(L"\nverticalPosDen=8") != std::wstring::npos);
	}

	void TestRoundTrip()
	{
		Config written;
		written.hideDelayMs = 1500;
		written.windowWidth = 200;
		written.windowHeight = 140;
		written.verticalPosNum = 3;
		written.verticalPosDen = 4;
		written.Save(g_path);

		Config loaded;
		loaded.Load(g_path);
		CHECK(loaded.hideDelayMs == 1500);
		CHECK(loaded.windowWidth == 200);
		CHECK(loaded.windowHeight == 140);
		CHECK(loaded.verticalPosNum == 3);
		CHECK(loaded.verticalPosDen == 4);
	}

	void TestCommentsWhitespaceAndUnknownKeys()
	{
		WriteFile(g_path,
				  L"; comment line with = sign\n"
				  L"# another comment\n"
				  L"\n"
				  L"  hideDelayMs = 2500  \r\n"
				  L"unknownKey=42\n"
				  L"windowWidth=180\n"
				  L"noEqualsSign\n");

		Config config;
		config.Load(g_path);
		CHECK(config.hideDelayMs == 2500);   // 两侧空白与 CR 都被裁掉
		CHECK(config.windowWidth == 180);
		CHECK(config.windowHeight == 112);   // 未出现的项保持默认值
	}

	void TestInvalidValuesKeepCurrentValue()
	{
		WriteFile(g_path,
				  L"hideDelayMs=abc\n"
				  L"windowWidth=-10\n"
				  L"windowHeight=120abc\n"
				  L"verticalPosNum=8\n");

		Config config;
		config.Load(g_path);
		CHECK(config.hideDelayMs == 1000);   // 非数字 -> 默认值
		CHECK(config.windowWidth == -10);    // 能解析出的负数先读入，由 Sanitize 兜底
		CHECK(config.windowHeight == 112);   // "120abc" 未整串消费 -> 默认值
		CHECK(config.verticalPosNum == 8);
		CHECK(config.verticalPosDen == 8);

		config.Sanitize();
		CHECK(config.windowWidth == 160);
	}

	void TestSavingAfterLoadKeepsValues()
	{
		Config first;
		first.hideDelayMs = 700;
		first.Save(g_path);

		Config second;
		second.Load(g_path);
		second.Save(g_path);

		Config third;
		third.Load(g_path);
		CHECK(third.hideDelayMs == 700);
	}

} // namespace

int main(int argc, char **argv)
{
	if (argc > 1 && argv[1] != nullptr && argv[1][0] != '\0')
	{
		const std::string narrow(argv[1]);
		g_path.assign(narrow.begin(), narrow.end());
	}

	TestDefaults();
	TestSanitizeRejectsNonPositiveValues();
	TestSaveCreatesFileWithDefaultsWhenMissing();
	TestRoundTrip();
	TestCommentsWhitespaceAndUnknownKeys();
	TestInvalidValuesKeepCurrentValue();
	TestSavingAfterLoadKeepsValues();

	RemoveFile(g_path);

	if (g_failures == 0)
	{
		std::printf("All Config tests passed.\n");
		return 0;
	}
	std::printf("%d check(s) failed.\n", g_failures);
	return 1;
}
