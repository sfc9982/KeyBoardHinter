#pragma once

// 应用配置持久化：把用户可调整项保存到 %APPDATA%/KeyBoardHinter/KeyBoardHinter.ini，
// 首次启动时自动生成默认配置文件。不依赖 wxWidgets，仅使用 Win32 与标准库。
//
// 实现见 Config.cpp；带路径参数的重载用于单元测试（可写入临时文件，不碰用户配置）。

#include <Windows.h>
#include <fstream>
#include <string>

struct Config {
	int hideDelayMs    = 1000;  // OSD 自动隐藏延迟（毫秒）
	int windowWidth    = 160;   // OSD 物理宽度（DIP）
	int windowHeight   = 112;   // OSD 物理高度（DIP）
	int verticalPosNum = 7;     // 纵向位置分子（位置 = 屏幕高度 * num / den）
	int verticalPosDen = 8;     // 纵向位置分母

	// 配置文件路径：%APPDATA%\KeyBoardHinter\KeyBoardHinter.ini（失败返回空串）。
	static std::wstring GetDefaultPath();

	// 从默认路径加载；文件不存在时写入默认配置。
	void Load();
	// 从指定路径加载，便于测试。
	void Load(const std::wstring &path);

	// 写入默认路径。
	void Save() const;
	// 写入指定路径，便于测试。
	void Save(const std::wstring &path) const;

	// 把越界值（<= 0）修正为内置默认值。
	void Sanitize();
};
