cmake_minimum_required(VERSION 3.16)

project(KeyBoardHinter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# wxWidgets 3.2.11 预编译包（vc14x, x64）位于本目录 wxWidgets/ 下：
#   include/                 通用头文件
#   include/msvc/wx/setup.h  MSVC 平台默认 setup.h
#   lib/vc14x_x64_dll/       库与 DLL（mswu = Release Unicode, mswud = Debug Unicode）
set(wxWidgets_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wxWidgets" CACHE PATH "wxWidgets 根目录")

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

# ---------------------------------------------------------------- 主程序 -----
# 分层：main 只做装配，UI/钩子/自启各自成模块，
# 纯逻辑（LockState / Config / Placement）不依赖 wxWidgets，可单独测试。
add_executable(KeyBoardHinter WIN32
    main.cpp            # 应用装配：单实例、DPI、钩子回调接线
    OSDWindow.cpp       # 悬浮卡片窗口与绘制
    TrayIcon.cpp        # 托盘图标与右键菜单
    KeyboardHook.cpp    # WH_KEYBOARD_LL 低级键盘钩子
    AutoStart.cpp       # 开机自启（注册表 Run 项）
    Config.cpp          # 配置读写
    LockState.cpp       # 锁定键状态机
)

# 让 IDE/VS 里也能看到头文件
target_sources(KeyBoardHinter PRIVATE
    OSDWindow.h
    TrayIcon.h
    KeyboardHook.h
    AutoStart.h
    Config.h
    LockState.h
    Placement.h
)

target_link_libraries(KeyBoardHinter PRIVATE ${wxWidgets_LIBRARIES})

if(MSVC)
    # UNICODE/_UNICODE 与 vcxproj 的 CharacterSet=Unicode 保持一致，
    # wxWidgets 的 Windows 构建要求 Unicode 宏。
    # /utf-8：源码为无 BOM 的 UTF-8（含中文注释与 u8 字面量），
    # 显式告知 MSVC，避免按本地代码页解释导致 C4819 与乱码。
    target_compile_definitions(KeyBoardHinter PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(KeyBoardHinter PRIVATE /utf-8)
endif()

# 把运行所需的 wxWidgets DLL 复制到可执行文件目录，
# 否则启动时会因找不到 wxmsw32u_core_vc14x_x64.dll 而失败。
if(WIN32)
    file(GLOB WX_DLLS "${wxWidgets_ROOT_DIR}/lib/vc14x_x64_dll/*.dll")
    add_custom_command(TARGET KeyBoardHinter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WX_DLLS} $<TARGET_FILE_DIR:KeyBoardHinter>
        COMMENT "复制 wxWidgets DLL 到输出目录")
endif()

# ---------------------------------------------------------------- 单元测试 ----
# 纯逻辑模块不依赖 wxWidgets 与窗口，零第三方依赖，由 CTest 驱动。
# 构建后运行：ctest --test-dir build -C Debug --output-on-failure
enable_testing()

# 锁定键状态机
add_executable(LockStateTest LockStateTest.cpp LockState.cpp)
add_test(NAME LockStateTest COMMAND LockStateTest)

# 配置读写（写入构建目录下的临时文件，不触碰用户真实配置）
add_executable(ConfigTest ConfigTest.cpp Config.cpp)
add_test(NAME ConfigTest
         COMMAND ConfigTest "${CMAKE_CURRENT_BINARY_DIR}/KeyBoardHinter.test.ini")

# 悬浮窗定位计算
add_executable(PlacementTest PlacementTest.cpp)
add_test(NAME PlacementTest COMMAND PlacementTest)

# OSD 卡片绘制：离屏渲染到位图后逐像素校验，不需要显示窗口。
# 依赖 wxWidgets 的 GUI 库（wxGraphicsContext）。
add_executable(OSDWindowTest OSDWindowTest.cpp OSDWindow.cpp LockState.cpp)
target_link_libraries(OSDWindowTest PRIVATE ${wxWidgets_LIBRARIES})
add_test(NAME OSDWindowTest COMMAND OSDWindowTest)

if(MSVC)
    foreach(test_target LockStateTest ConfigTest PlacementTest OSDWindowTest)
        target_compile_definitions(${test_target} PRIVATE _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
        target_compile_options(${test_target} PRIVATE /utf-8)
    endforeach()
endif()
