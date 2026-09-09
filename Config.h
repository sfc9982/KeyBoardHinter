#pragma once

// 应用配置持久化：把用户可调整项保存到 %APPDATA%/KeyBoardHinter/KeyBoardHinter.ini，
// 首次启动时自动生成默认配置文件。不依赖 wxWidgets，仅使用 Win32 与标准库。

#include <Windows.h>
#include <cwchar>
#include <fstream>
#include <string>

struct Config {
	int hideDelayMs   = 1000;
	int windowWidth   = 160;
	int windowHeight  = 112;
	int verticalPosNum = 7;
	int verticalPosDen = 8;

	// 从配置文件加载；文件不存在时写入默认配置。
	void Load() {
		std::wstring path = GetConfigPath();
		if (path.empty()) return;
		std::wifstream file(path.c_str());
		if (!file.is_open()) {
			Save();
			return;
		}
		std::wstring line;
		while (std::getline(file, line)) {
			Trim(line);
			if (line.empty() || line.front() == L';' || line.front() == L'#') continue;
			const auto pos = line.find(L'=');
			if (pos == std::wstring::npos) continue;
			std::wstring key   = line.substr(0, pos);
			std::wstring value = line.substr(pos + 1);
			Trim(key);
			Trim(value);
			if (key == L"hideDelayMs")    hideDelayMs   = ParseInt(value, hideDelayMs);
			else if (key == L"windowWidth")   windowWidth   = ParseInt(value, windowWidth);
			else if (key == L"windowHeight")  windowHeight  = ParseInt(value, windowHeight);
			else if (key == L"verticalPosNum") verticalPosNum = ParseInt(value, verticalPosNum);
			else if (key == L"verticalPosDen") verticalPosDen = ParseInt(value, verticalPosDen);
		}
	}

	// 将当前配置写入文件。
	void Save() const {
		std::wstring path = GetConfigPath();
		if (path.empty()) return;
		std::wofstream file(path.c_str());
		if (!file.is_open()) return;
		file << L"; KeyBoardHinter configuration\n";
		file << L"; Available keys: hideDelayMs, windowWidth, windowHeight, verticalPosNum, verticalPosDen\n";
		file << L"hideDelayMs=" << hideDelayMs << L"\n";
		file << L"windowWidth=" << windowWidth << L"\n";
		file << L"windowHeight=" << windowHeight << L"\n";
		file << L"verticalPosNum=" << verticalPosNum << L"\n";
		file << L"verticalPosDen=" << verticalPosDen << L"\n";
	}

private:
	static std::wstring GetConfigPath() {
		wchar_t buffer[MAX_PATH];
		const DWORD length = ::GetEnvironmentVariableW(L"APPDATA", buffer, MAX_PATH);
		if (length == 0 || length >= MAX_PATH) return {};
		std::wstring dir = std::wstring(buffer, length) + L"\\KeyBoardHinter";
		::CreateDirectoryW(dir.c_str(), nullptr);
		return dir + L"\\KeyBoardHinter.ini";
	}

	static int ParseInt(const std::wstring &text, int fallback) {
		try {
			size_t pos = 0;
			int value = std::stoi(text, &pos);
			if (pos == text.size()) return value;
		} catch (...) {
		}
		return fallback;
	}

	static void Trim(std::wstring &text) {
		while (!text.empty() && (text.front() == L' ' || text.front() == L'\t')) {
			text.erase(text.begin());
		}
		while (!text.empty() && (text.back() == L' ' || text.back() == L'\t')) {
			text.pop_back();
		}
	}
};
