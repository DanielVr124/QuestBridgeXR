cmake_minimum_required(VERSION 3.24)
project(QuestBridgeOpenXRRuntime LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "QuestBridgeOpenXRRuntime is Windows-only (D3D11 + Win32 + Winsock).")
endif()

include(FetchContent)
set(BUILD_LOADER OFF CACHE BOOL "Do not build the OpenXR loader" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Do not build OpenXR SDK tests" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "Do not build OpenXR conformance tests" FORCE)
set(BUILD_API_LAYERS OFF CACHE BOOL "Do not build API layers" FORCE)
set(BUILD_SDK_TESTS OFF CACHE BOOL "Do not build OpenXR SDK samples" FORCE)
FetchContent_Declare(openxr_sdk
  GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
  GIT_TAG release-1.1.63
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(openxr_sdk)

add_library(qbx_openxr_runtime SHARED src/qbx_runtime.cpp)
target_compile_features(qbx_openxr_runtime PRIVATE cxx_std_20)
target_include_directories(qbx_openxr_runtime PRIVATE
  ${openxr_sdk_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../protocol/include)
target_link_libraries(qbx_openxr_runtime PRIVATE d3d11 dxgi dxguid d3dcompiler ws2_32)
if(MSVC)
  target_compile_options(qbx_openxr_runtime PRIVATE /W4 /permissive- /EHsc /Zc:__cplusplus)
  set_property(TARGET qbx_openxr_runtime PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
set_target_properties(qbx_openxr_runtime PROPERTIES OUTPUT_NAME "qbx_openxr_runtime")

install(TARGETS qbx_openxr_runtime RUNTIME DESTINATION runtime LIBRARY DESTINATION runtime)
install(FILES manifest/questbridge_openxr_runtime.json DESTINATION runtime)
