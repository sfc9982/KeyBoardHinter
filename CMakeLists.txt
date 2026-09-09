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

add_executable(KeyBoardHinter WIN32 KeyBoardHinter.cpp)

target_link_libraries(KeyBoardHinter PRIVATE ${wxWidgets_LIBRARIES})

if(MSVC)
    # 与 vcxproj 的 CharacterSet=Unicode 保持一致，
    # wxWidgets 的 Windows 构建要求 Unicode 宏。
    target_compile_definitions(KeyBoardHinter PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
endif()

# 把运行所需的 wxWidgets DLL 复制到可执行文件目录，
# 否则启动时会因找不到 wxmsw32u_core_vc14x_x64.dll 而失败。
if(WIN32)
    file(GLOB WX_DLLS "${wxWidgets_ROOT_DIR}/lib/vc14x_x64_dll/*.dll")
    add_custom_command(TARGET KeyBoardHinter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WX_DLLS} $<TARGET_FILE_DIR:KeyBoardHinter>
        COMMENT "复制 wxWidgets DLL 到输出目录")
endif()

# 单元测试：锁定键状态机（LockState）不依赖 wxWidgets 与 Win32 UI，零第三方依赖。
# 构建后运行：ctest --test-dir build -C Debug
enable_testing()
add_executable(LockStateTest LockStateTest.cpp)
target_compile_definitions(LockStateTest PRIVATE _CRT_SECURE_NO_WARNINGS)
add_test(NAME LockStateTest COMMAND LockStateTest)
