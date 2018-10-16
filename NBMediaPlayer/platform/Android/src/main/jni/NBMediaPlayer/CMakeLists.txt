cmake_minimum_required(VERSION 3.0)
project(NBMediaPlayer)

set(CMAKE_CXX_STANDARD 98)

set(SRC_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)
set(NBMEIDAPLAYER_ROOT ${SRC_TOP_DIR})
set(DOWNLOAD_ENGINE_ROOT ${NBMEIDAPLAYER_ROOT}/src/DownloadEngine)
set(CUSTOM_FFMPEG_ROOT ${NBMEIDAPLAYER_ROOT}/src/datasource/ffmpeg)

include_directories(
        ${NBMEIDAPLAYER_ROOT}/include
        ${NBMEIDAPLAYER_ROOT}/src
        ${NBMEIDAPLAYER_ROOT}/src/datasource/ffmpeg
        ${NBMEIDAPLAYER_ROOT}/src/DownloadEngine/include
        ${NBMEIDAPLAYER_ROOT}/deps/Android/openal-soft/include
        ${DOWNLOAD_ENGINE_ROOT}/include
        ${DOWNLOAD_ENGINE_ROOT}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/LibAndroid
)

add_compile_options(-Wno-multichar)

set(SRC_DOWNLOAD_ENGINE
        ${DOWNLOAD_ENGINE_ROOT}/src/DEDownloadEngine.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/common/DELooperEV.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/filesystem/DEDataManager.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/filesystem/DEMemDataManager.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/network/DENetwork.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/network/DESocketHelper.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/task/DEBaseTask.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/task/DECurlHttpTask.cpp
        ${DOWNLOAD_ENGINE_ROOT}/src/task/DECurlTask.cpp
        )

set(SRC_CUSTOM_FFMPEG
        ${CUSTOM_FFMPEG_ROOT}/libavcodec/decode.c
        ${CUSTOM_FFMPEG_ROOT}/libavcodec/h264_parse.c
        ${CUSTOM_FFMPEG_ROOT}/libavcodec/h264_ps.c
        ${CUSTOM_FFMPEG_ROOT}/libavcodec/h264_slice.c
        ${CUSTOM_FFMPEG_ROOT}/libavcodec/h264dec.c
        ${CUSTOM_FFMPEG_ROOT}/libavcodec/h2645_parse.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/avio.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/file.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/http.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/httpauth.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/protocols.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rdt.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtp.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_ac3.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_amr.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_asf.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_dv.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_g726.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_h261.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_h263_rfc2190.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_h263.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_h264.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_hevc.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_ilbc.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_jpeg.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_latm.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_mpa_robust.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_mpeg4.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_mpeg12.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_mpegts.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_qcelp.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_qdm2.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_qt.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_rfc4175.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_svq3.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_vc2hq.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_vp8.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_vp9.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec_xiph.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpdec.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtpproto.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtsp.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/rtspdec.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/srtp.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/srtpproto.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/tcp.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/udp.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/urldecode.c
        ${CUSTOM_FFMPEG_ROOT}/libavformat/utils.c
    )

set(SRC_SET
        ${NBMEIDAPLAYER_ROOT}/src/NBMediaPlayer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/NBMediaBuffer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/NBAVPacket.cpp
        ${NBMEIDAPLAYER_ROOT}/src/NBAudioFrame.cpp
        ${NBMEIDAPLAYER_ROOT}/src/NBVideoFrame.cpp
        ${NBMEIDAPLAYER_ROOT}/src/NBBufferQueue.cpp
        ${NBMEIDAPLAYER_ROOT}/src/NBAudioPlayer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/NBClockManager.cpp
        ${NBMEIDAPLAYER_ROOT}/src/foundation/NBString.cpp
        ${NBMEIDAPLAYER_ROOT}/src/foundation/NBSharedBuffer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/foundation/NBAtomic.cpp
        ${NBMEIDAPLAYER_ROOT}/src/foundation/NBRBTree.cpp
        ${NBMEIDAPLAYER_ROOT}/src/foundation/NBTimedEventQueue.cpp
        ${NBMEIDAPLAYER_ROOT}/src/foundation/NBMetaData.cpp
        ${NBMEIDAPLAYER_ROOT}/src/foundation/NBFoundation.cpp
        ${NBMEIDAPLAYER_ROOT}/src/datasource/NBDataSource.cpp
        ${NBMEIDAPLAYER_ROOT}/src/datasource/NBFFmpegSource.cpp
        ${NBMEIDAPLAYER_ROOT}/src/datasource/NBBeastSource.cpp
        ${NBMEIDAPLAYER_ROOT}/src/extractor/NBMediaExtractor.cpp
        ${NBMEIDAPLAYER_ROOT}/src/extractor/NBFFmpegExtractor.cpp
        ${NBMEIDAPLAYER_ROOT}/src/decoder/NBMediaSource.cpp
        ${NBMEIDAPLAYER_ROOT}/src/decoder/NBMediaDecoder.cpp
        ${NBMEIDAPLAYER_ROOT}/src/decoder/NBFFmpegMediaTrack.cpp
        ${NBMEIDAPLAYER_ROOT}/src/decoder/NBFFmpegVDecoder.cpp
        ${NBMEIDAPLAYER_ROOT}/src/decoder/NBFFmpegADecoder.cpp
        ${NBMEIDAPLAYER_ROOT}/src/decoder/NBMediaCodecVDecoder.cpp
        ${NBMEIDAPLAYER_ROOT}/src/renderer/NBALAudioRenderer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/renderer/NBVideoRenderer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/renderer/NBMediaCodecVRenderer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/renderer/NBGLVideoRenderer.cpp
        ${NBMEIDAPLAYER_ROOT}/src/renderer/NBGLShaderUtils.cpp
        ${NBMEIDAPLAYER_ROOT}/src/renderer/NBGLYUV420PRenderer.cpp
        ${SRC_DOWNLOAD_ENGINE}
        ${SRC_CUSTOM_FFMPEG}
        )

add_library(NBMediaPlayer STATIC ${SRC_SET})
target_link_libraries(NBMediaPlayer
        avformat
        avcodec
        swresample
        swscale
        avutil
        libcurl
        openal-soft
        libev
        LibAndroid
        ${BUILD_DEPS}
        dl
        z)

set_property(TARGET NBMediaPlayer PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(deps)