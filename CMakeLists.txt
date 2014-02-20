cmake_minimum_required(VERSION 2.6)
project(fftproj)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt4 REQUIRED)
set(CMAKE_AUTOMOC ON)
SET(QT_USE_QTMULTIMEDIA TRUE)
include(${QT_USE_FILE})

add_library(clhelpers
	clhelpers/check_err.cpp
	clhelpers/device.cpp
	clhelpers/platform.cpp
)

add_executable(tuner
	tuner/main.cpp
	tuner/SetupDialog.cpp

	dft-interface/DftAlgorithm.cpp
	dft-interface/DftAlgorithmManager.cpp
	dft-interface/Serial.cpp
)
target_link_libraries(tuner OpenCL clhelpers ${QT_LIBRARIES})

# Disattiva automoc per i test (che usano solo la STL, non Qt4)
set(CMAKE_AUTOMOC OFF)

add_executable(test-serial_naive_dft dft-tests/test-serial.cpp)
set_target_properties(test-serial_naive_dft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_naive_dft -DALLOW_NPOT")

add_executable(test-serial_recursive_fft dft-tests/test-serial.cpp)
set_target_properties(test-serial_recursive_fft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_recursive_fft -DENABLE_CHECK")

add_executable(test-serial_nonrecursive_fft dft-tests/test-serial.cpp)
set_target_properties(test-serial_nonrecursive_fft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_nonrecursive_fft -DENABLE_CHECK")
