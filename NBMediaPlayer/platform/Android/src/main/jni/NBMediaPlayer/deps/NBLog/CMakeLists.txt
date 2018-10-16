cmake_minimum_required(VERSION 3.0.0)
project(NBLog C)

#set(CMAKE_C_STANDARD 99)

set(NBLOG_ROOT ${SRC_TOP_DIR}/deps/NBLog)

include_directories(
        ${NBLOG_ROOT}/include
)

set(SRC_SET
        ${NBLOG_ROOT}/src/NBLog.c
        ${NBLOG_ROOT}/src/NBLogAppender.c
        ${NBLOG_ROOT}/src/NBLogMFileAppender.c
        ${NBLOG_ROOT}/src/NBLogConsoleAppender.c)

add_library(NBLog ${SRC_SET})
set_property(TARGET NBLog PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${NBLOG_ROOT}/include")
set_property(TARGET NBLog PROPERTY POSITION_INDEPENDENT_CODE ON)