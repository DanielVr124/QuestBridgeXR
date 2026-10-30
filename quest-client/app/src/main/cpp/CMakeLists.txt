cmake_minimum_required(VERSION 3.22.1)
project(qbxquest LANGUAGES C CXX)

find_package(OpenXR REQUIRED CONFIG)

if(DEFINED CMAKE_ANDROID_NDK)
    set(QBX_NDK_ROOT "${CMAKE_ANDROID_NDK}")
else()
    set(QBX_NDK_ROOT "${ANDROID_NDK}")
endif()
set(NATIVE_APP_GLUE "${QBX_NDK_ROOT}/sources/android/native_app_glue/android_native_app_glue.c")

add_library(qbxquest SHARED
    main.cpp
    video_decoder.cpp
    room_renderer.cpp
    glb_model.cpp
    "${NATIVE_APP_GLUE}"
)

target_compile_features(qbxquest PRIVATE cxx_std_20)
target_compile_definitions(qbxquest PRIVATE
    XR_USE_PLATFORM_ANDROID
    XR_USE_GRAPHICS_API_OPENGL_ES
    GL_GLEXT_PROTOTYPES=1
    QBX_PROTOCOL_VERSION=1
)
target_include_directories(qbxquest PRIVATE
    "${QBX_NDK_ROOT}/sources/android/native_app_glue"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../protocol/include"
)
target_link_libraries(qbxquest PRIVATE
    OpenXR::openxr_loader
    android log EGL GLESv3 mediandk
)
target_compile_options(qbxquest PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
