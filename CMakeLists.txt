cmake_minimum_required(VERSION 3.16)
project(MouseBatteryTray CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MouseBatteryTray WIN32 standalone/main.cpp)
target_compile_definitions(MouseBatteryTray PRIVATE UNICODE _UNICODE)
target_link_libraries(MouseBatteryTray PRIVATE setupapi hid gdi32 shell32 advapi32 user32)

if(MINGW)
  target_link_options(MouseBatteryTray PRIVATE -municode -static)
elseif(MSVC)
  target_compile_definitions(MouseBatteryTray PRIVATE _CRT_SECURE_NO_WARNINGS)
  set_property(TARGET MouseBatteryTray PROPERTY
               MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Protocol parser tests (pure C++, no Windows APIs).
add_executable(parsers_test tests/parsers_test.cpp)
