#include "Config.h"

#include <utility>

namespace {

	void Trim(std::wstring &text)
	{
		while (!text.empty() && (text.front() == L' ' || text.front() == L'\t'))
		{
			text.erase(text.begin());
		}
		while (!text.empty() && (text.back() == L' ' || text.back() == L'\t' || text.back() == L'\r'))
		{
			text.pop_back();
		}
	}

	int ParseInt(const std::wstring &text, int fallback)
	{
		try
		{
			size_t pos = 0;
			const int value = std::stoi(text, &pos);
			// 要求整串都被消费，避免 "1000abc" 这类脏数据被当成 1000
			if (pos == text.size())
			{
				return value;
			}
		}
		catch (...)
		{
		}
		return fallback;
	}

} // namespace

std::wstring Config::GetDefaultPath()
{
	wchar_t buffer[MAX_PATH];
	const DWORD length = ::GetEnvironmentVariableW(L"APPDATA", buffer, MAX_PATH);
	if (length == 0 || length >= MAX_PATH)
	{
		return {};
	}
	const std::wstring dir = std::wstring(buffer, length) + L"\\KeyBoardHinter";
	::CreateDirectoryW(dir.c_str(), nullptr);
	return dir + L"\\KeyBoardHinter.ini";
}

void Config::Load()
{
	Load(GetDefaultPath());
}

void Config::Load(const std::wstring &path)
{
	if (path.empty())
	{
		return;
	}
	std::wifstream file(path.c_str());
	if (!file.is_open())
	{
		// 首次运行（或文件被删除）：落盘一份带注释的默认配置，方便用户直接编辑
		Save(path);
		return;
	}

	std::wstring line;
	while (std::getline(file, line))
	{
		Trim(line);
		if (line.empty() || line.front() == L';' || line.front() == L'#')
		{
			continue;
		}
		const auto pos = line.find(L'=');
		if (pos == std::wstring::npos)
		{
			continue;
		}
		std::wstring key = line.substr(0, pos);
		std::wstring value = line.substr(pos + 1);
		Trim(key);
		Trim(value);
		// 每一项都以当前值为兜底：非法值退化为"保持现值"，不会把配置清成 0
		if (key == L"hideDelayMs") { hideDelayMs = ParseInt(value, hideDelayMs); }
		else if (key == L"windowWidth") { windowWidth = ParseInt(value, windowWidth); }
		else if (key == L"windowHeight") { windowHeight = ParseInt(value, windowHeight); }
		else if (key == L"verticalPosNum") { verticalPosNum = ParseInt(value, verticalPosNum); }
		else if (key == L"verticalPosDen") { verticalPosDen = ParseInt(value, verticalPosDen); }
	}
}

void Config::Save() const
{
	Save(GetDefaultPath());
}

void Config::Save(const std::wstring &path) const
{
	if (path.empty())
	{
		return;
	}
	std::wofstream file(path.c_str());
	if (!file.is_open())
	{
		return;
	}
	// 注意：注释必须保持纯 ASCII。MSVC 下 std::wofstream 会把宽字符按当前 locale
	// 转换成本地代码页字节，遇到非 ASCII 字符会置 badbit 并**静默丢弃后续所有写入**
	// （vswprintf 会返回 -1）。因此配置文件内容一律使用英文，中文只出现在源码注释里。
	file << L"; KeyBoardHinter configuration\n";
	file << L"; Available keys: hideDelayMs, windowWidth, windowHeight, verticalPosNum, verticalPosDen\n";
	file << L"; hideDelayMs     how long the OSD stays visible, in milliseconds\n";
	file << L"; windowWidth     OSD width in DIP (device independent pixels)\n";
	file << L"; windowHeight    OSD height in DIP\n";
	file << L"; verticalPosNum  vertical position numerator (screen height * num / den)\n";
	file << L"; verticalPosDen  vertical position denominator\n";
	file << L"hideDelayMs=" << hideDelayMs << L"\n";
	file << L"windowWidth=" << windowWidth << L"\n";
	file << L"windowHeight=" << windowHeight << L"\n";
	file << L"verticalPosNum=" << verticalPosNum << L"\n";
	file << L"verticalPosDen=" << verticalPosDen << L"\n";
}

void Config::Sanitize()
{
	if (hideDelayMs <= 0) { hideDelayMs = 1000; }
	if (windowWidth <= 0) { windowWidth = 160; }
	if (windowHeight <= 0) { windowHeight = 112; }
	if (verticalPosNum <= 0) { verticalPosNum = 7; }
	if (verticalPosDen <= 0) { verticalPosDen = 8; }
}
