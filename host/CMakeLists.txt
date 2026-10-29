cmake_minimum_required(VERSION 3.20)
project(QuestBridgeHost LANGUAGES CXX)

# Prefer static MSVC CRT linkage so the portable host is less dependent on a
# separately installed Visual C++ Redistributable. (Windows SDK libraries remain system dependencies.)

if(NOT WIN32)
  message(FATAL_ERROR "QuestBridgeHost is currently a Windows-only Winsock prototype.")
endif()

add_executable(QuestBridgeHost src/main.cpp src/fake_vr_bridge.cpp)
target_compile_features(QuestBridgeHost PRIVATE cxx_std_20)
target_include_directories(QuestBridgeHost PRIVATE ../protocol/include)
target_compile_definitions(QuestBridgeHost PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(QuestBridgeHost PRIVATE ws2_32)
if(MSVC)
  set_property(TARGET QuestBridgeHost PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(MSVC)
  target_compile_options(QuestBridgeHost PRIVATE /W4 /permissive- /EHsc)
endif()
