cmake_minimum_required(VERSION 2.6)
project(fftproj)

find_package(Qt4 REQUIRED)
set(CMAKE_AUTOMOC ON)
SET(QT_USE_QTMULTIMEDIA TRUE)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(tuner
	tuner/main.cpp
	tuner/SetupDialog.cpp
)

add_library(clhelpers
	clhelpers/check_err.cpp
	clhelpers/device.cpp
	clhelpers/platform.cpp
)

target_link_libraries(tuner OpenCL clhelpers ${QT_LIBRARIES})
