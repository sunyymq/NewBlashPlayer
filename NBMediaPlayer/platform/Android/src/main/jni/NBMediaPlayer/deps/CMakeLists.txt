#export the ffmpeg
cmake_minimum_required(VERSION 2.8.7)
project(deps)

# build ffmpeg begin

set(FFMPEG_ROOT ${SRC_TOP_DIR}/deps/Android/ffmpeg/armeabi-v7a)

add_library(avformat STATIC IMPORTED GLOBAL)
set_target_properties(avformat PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_ROOT}/lib/libavformat.a
        INTERFACE_INCLUDE_DIRECTORIES ${FFMPEG_ROOT}/include)

add_library(avcodec STATIC IMPORTED GLOBAL)
set_target_properties(avcodec PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_ROOT}/lib/libavcodec.a
        INTERFACE_INCLUDE_DIRECTORIES ${FFMPEG_ROOT}/include)

add_library(avutil STATIC IMPORTED GLOBAL)
set_target_properties(avutil PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_ROOT}/lib/libavutil.a
        INTERFACE_INCLUDE_DIRECTORIES ${FFMPEG_ROOT}/include)

add_library(swresample STATIC IMPORTED GLOBAL)
set_target_properties(swresample PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_ROOT}/lib/libswresample.a
        INTERFACE_INCLUDE_DIRECTORIES ${FFMPEG_ROOT}/include)

add_library(swscale STATIC IMPORTED GLOBAL)
set_target_properties(swscale PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_ROOT}/lib/libswscale.a
        INTERFACE_INCLUDE_DIRECTORIES ${FFMPEG_ROOT}/include)

# build ffmpeg end

# build curl begin
set(LIBCURL_ROOT ${SRC_TOP_DIR}/deps/Android/curl/armeabi-v7a)
add_library(libcurl STATIC IMPORTED GLOBAL)
set_target_properties(libcurl PROPERTIES
        IMPORTED_LOCATION ${LIBCURL_ROOT}/lib/libcurl.a
        INTERFACE_INCLUDE_DIRECTORIES ${LIBCURL_ROOT}/include)
# build curl end

# build libev begin
set(LIBEV_ROOT ${SRC_TOP_DIR}/deps/Android/libev/armeabi-v7a)
add_library(libev STATIC IMPORTED GLOBAL)
set_target_properties(libev PROPERTIES
        IMPORTED_LOCATION ${LIBEV_ROOT}/lib/libev.a
        INTERFACE_INCLUDE_DIRECTORIES ${LIBEV_ROOT}/include)
# build libev end

# build openal-soft begin
set(OPENAL_SOFT_ROOT ${SRC_TOP_DIR}/deps/Android/openal-soft/armeabi-v7a)
add_library(openal-soft STATIC IMPORTED GLOBAL)
set_target_properties(openal-soft PROPERTIES
        IMPORTED_LOCATION ${OPENAL_SOFT_ROOT}/lib/libopenal.a
        INTERFACE_INCLUDE_DIRECTORIES ${OPENAL_SOFT_ROOT}/include)
# build openal-soft end


# build openal-soft
#set(ALSOFT_DLOPEN OFF CACHE BOOL "disable dlopen")
#set(ALSOFT_UTILS OFF CACHE BOOL "disable utils")
#set(ALSOFT_EXAMPLES OFF CACHE BOOL "disable examples")
#set(ALSOFT_TESTS OFF CACHE BOOL "disable tests")

add_subdirectory(NBLog)
add_subdirectory(LibAndroid)