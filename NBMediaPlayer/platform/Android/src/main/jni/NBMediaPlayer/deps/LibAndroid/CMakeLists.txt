cmake_minimum_required(VERSION 3.0.0)
project(LibAndroid CXX)

set(CMAKE_CXX_STANDARD 98)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set(SRC_SET
        AndroidArraylist.cpp
        AndroidBuild.cpp
        AndroidBundle.cpp
        AndroidBytebuffer.cpp
        AndroidHashMap.cpp
        AndroidInteger.cpp
        AndroidJni.cpp
        AndroidLong.cpp
        AndroidMediacodec.cpp
        AndroidMediacodecJava.cpp
        AndroidMediadef.cpp
        AndroidMediaformat.cpp
        AndroidMediaformatJava.cpp
        AndroidString.cpp
        AndroidSurface.cpp
        AndroidSurfaceTexture.cpp
        JNIUtils.cpp)

add_library(LibAndroid ${SRC_SET})
set_property(TARGET LibAndroid PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
set_property(TARGET LibAndroid PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(LibAndroid
                       NBLog)