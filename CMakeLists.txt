cmake_minimum_required(VERSION 2.6)
project(fftproj)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt4 REQUIRED)
set(CMAKE_AUTOMOC ON)
SET(QT_USE_QTMULTIMEDIA TRUE)
include(${QT_USE_FILE})

add_library(clhelpers
	clhelpers/align.cpp
	clhelpers/check_err.cpp
	clhelpers/command_queue.cpp
	clhelpers/context.cpp
	clhelpers/device.cpp
	clhelpers/event.cpp
	clhelpers/kernel.cpp
	clhelpers/nvidia_cache.cpp
	clhelpers/platform.cpp
	clhelpers/program.cpp
)

add_executable(tuner
	tuner/main.cpp
	tuner/SetupDialog.cpp

	dft-interface/CLAlgorithm.cpp
	dft-interface/DftAlgorithm.cpp
	dft-interface/DftAlgorithmManager.cpp
	dft-interface/SerialAlgorithm.cpp
)
target_link_libraries(tuner OpenCL clhelpers ${QT_LIBRARIES})

# Disattiva automoc per i test (che usano solo la STL, non Qt4)
set(CMAKE_AUTOMOC OFF)

add_executable(ram_speed tools/ram_speed.cpp)

add_executable(cl_speed tools/cl_speed.cpp)
target_link_libraries(cl_speed OpenCL clhelpers)

add_executable(access_patterns tools/access_patterns.cpp)
target_link_libraries(access_patterns OpenCL clhelpers)

add_executable(triangle_grid tools/triangle_grid.cpp)
target_link_libraries(triangle_grid OpenCL clhelpers)

add_executable(test-serial_naive_dft dft-tests/test-serial.cpp)
set_target_properties(test-serial_naive_dft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_naive_dft -DALLOW_NPOT")

add_executable(test-serial_recursive_fft dft-tests/test-serial.cpp)
set_target_properties(test-serial_recursive_fft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_recursive_fft -DENABLE_CHECK")

add_executable(test-serial_nonrecursive_fft dft-tests/test-serial.cpp)
set_target_properties(test-serial_nonrecursive_fft PROPERTIES COMPILE_FLAGS "-DALGOFUNC=serial_nonrecursive_fft -DENABLE_CHECK")

add_executable(test-cl_naive_dft dft-tests/test-cl.cpp)
set_target_properties(test-cl_naive_dft PROPERTIES COMPILE_FLAGS "-DALGOCLASS=cl_naive_dft -DALLOW_NPOT")
target_link_libraries(test-cl_naive_dft OpenCL clhelpers)

add_executable(test-cl_dft_matrix dft-tests/test-cl.cpp)
set_target_properties(test-cl_dft_matrix PROPERTIES COMPILE_FLAGS "-DALGOCLASS=cl_dft_matrix -DALLOW_NPOT")
target_link_libraries(test-cl_dft_matrix OpenCL clhelpers)

add_executable(test-cl_dft_symmatrix dft-tests/test-cl.cpp)
set_target_properties(test-cl_dft_symmatrix PROPERTIES COMPILE_FLAGS "-DALGOCLASS=cl_dft_symmatrix -DALLOW_NPOT")
target_link_libraries(test-cl_dft_symmatrix OpenCL clhelpers)
